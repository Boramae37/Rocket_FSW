#include <iostream>
#include <vector>
#include <string>
#include <cstring>


#include <sys/socket.h> // socket(), connect(), write()
#include <sys/un.h>    // struct sockaddr_un
#include <cstdlib>     // exit()
#include <cstdio>      // snprintf()
#include <charconv>   // std::to_chars


// i2c를 위한.. linux kernel 이용하는 거임
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>  // ioctl()
#include <fcntl.h>      // open(), O_RDWR
#include <unistd.h>     // read(), write(), close(), usleep()

// threading 
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>

// fmt
#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/chrono.h"
#include "spdlog/fmt/ranges.h" // join, range

// my includes
#include "sensor.h"
#include "connection.h" // for communication with software bus


// PMCCNTR_EL0 읽기 함수
// static inline uint64_t read_pmccntr(void) {
//     uint64_t cc = 0;
//     asm volatile("mrs %0, pmccntr_el0" : "=r"(cc));
//     return cc;
// }

// static inline uint64_t read_time_ns(void) {
//     struct timespec ts;
//     clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
//     return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
// }


// function declarations
bool vector_to_string(const std::vector<float>& data, std::string& str);
void thread_read_sensor(Sensor& sensor, int period, int sensor_id);



// global variables
std::mutex i2c_mutex; // i2c bus mutex
std::mutex sb_mutex; // software bus mutex
std::atomic<bool> keep_running(true);


///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////



int sockfd; // socket file descriptor

int main(){

  auto now_time = std::chrono::system_clock::now();
  fmt::print("SENSOR APP STARTED at {}\n", fmt::format("{:%Y-%m-%d %H:%M:%S.%e}", now_time));

  //////////////////////////////////////////////////////////////////////////
  // Software Bus 연결을 위한 공통 부분(물론 SENSOR_ID등 등록 부분은 바꿔야함)
  //////////////////////////////////////////////////////////////////////////


  // Socket setup
  int ret; // return value
  // int sockfd; // socket file descriptor
  struct sockaddr_un addr;
  char buffer[SB_BUFFER_SIZE];
  sockfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);
  connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));

  // Sensor APP FD를 SB에 등록
  Message msg(SENSOR_ID);
  msg.set_message_id(SET_FD_COMMAND);
  snprintf(buffer, sizeof(buffer), "%d", sockfd);
  msg.set_data(buffer);
  write(sockfd, msg.get_message(), SB_BUFFER_SIZE);

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
  


// Sensor setup
  BMP390 baro;
  BNO055 imu;
  ULTIMATEGPS gps(9600, 27, 22);

  std::vector<float> baro_data;
  std::vector<float> imu_data;
  

  baro.init();
  usleep(1000000); // 1 sec
  imu.init();
  usleep(1000000); // 1 sec
  
  char temp_msg[SB_BUFFER_SIZE] = "0,7,SENSOR APP READY, WAITING FOR A COMMAND";
  fmt::print("SENSOR APP READ. Waiting Command\n");
  write(sockfd, temp_msg, SB_BUFFER_SIZE);


  while(1){
    
    // unix domain socket에서 read 기본적으로 blocking 되어 있음.
    ret = read(sockfd, buffer, SB_BUFFER_SIZE);
    buffer[ret] = '\0';
    ret = strlen(buffer);
    buffer[ret] = '\0'; // null-terminate the string 
    fmt::print("SENSOR read buffer : size {} : {}\n", ret,buffer);
  
    
    if((buffer[2] - '0' == COMMAND_MESSAGEID)&&(buffer[4] - '0' == START_COMMAND)){
      fmt::print("[SESOR] START COMMAND RECEIVED, STOP SENSOR APP\n");
      break;
    } 
    memset(buffer, 0, SB_BUFFER_SIZE); 
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  
  
  // Sensor threads
  // 센서 ID는 "connection.h"에서 정의된 상수 사용
  fmt::print("[SENSOR] Starting Barometer threads...\n");
  std::thread barometer_thread(
    [&] { thread_read_sensor (std::ref(baro), 10, BARO_ID); }
  );
  fmt::print("[SENSOR] Starting IMU threads...\n");
  std::thread imu_thread(
    [&] { thread_read_sensor(std::ref(imu), 12, IMU_ID); }
  );

  // SB에서 오는 명령 처리하는 메인 스레드 파트.. (0.5초마다 확인 예정.)
  while(1){
    read(sockfd, buffer, SB_BUFFER_SIZE);
    ret = strlen(buffer);
    buffer[ret] = '\0'; // null-terminate the string
    if((buffer[2] - '0' == COMMAND_MESSAGEID)&&(buffer[5] - '0' == STOP_COMMAND)){
      fmt::print("[SESOR] STOP COMMAND RECEIVED, STOP SENSOR APP\n");
      snprintf(buffer, SB_BUFFER_SIZE, "%d,%d,%d\n", SENSOR_ID, INFO_ID, STOP_COMMAND);
      //ret = strlen(buffer);
      write(sockfd, buffer, SB_BUFFER_SIZE); // sb_buffer_size는 고정. 왜냐면 SB에서 읽을때 고정 길이로 읽음.
      //keep_running = false; // stop the sensor threads
      break; // exit the loop
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));
  keep_running = false;
  

  barometer_thread.join();
  imu_thread.join();
  
  fmt::print("[SENSOR] Sensor threads stopped.\n");
  // sensor 죽어요 메세지 보내고, sb에서느 각 앱 죽는지 모두 확인하고, 종료하면 될듯?
  return 0;
}


// 전체 string 길이 반환(null제외) -> fmt::join으로 대체
// int vector_to_string(const std::vector<float>& values, char* out_buf) {
//   char* ptr = out_buf;
//   char* buf_end = out_buf + SB_BUFFER_SIZE;
//   int string_length = 0;

//   for (size_t i = 0; i < values.size(); ++i) {
//     char num_buf[32];
//     auto [num_end, ec] = std::to_chars(num_buf, num_buf + sizeof(num_buf), values[i]);
//     if (ec != std::errc()) return -1;

//     size_t len = num_end - num_buf;
//     if (ptr + len >= buf_end) return -1;

//     std::memcpy(ptr, num_buf, len);
//     ptr += len;
//     string_length += len;
    
//     if (i + 1 < values.size()) {
//         if (ptr + 1 >= buf_end) return -1;
//         *ptr++ = ',';
//     }
//   }

//   if (ptr >= buf_end) return -1;
//   *ptr = '\0';
//   return string_length;
// }


void thread_read_sensor(Sensor& sensor, int period, int sensor_id){
  std::vector<float> data;
  Message msg(SENSOR_ID); // sensor_id is the specific sensor id (IMU or Barometer)
  msg.set_message_id(sensor_id);
  char data_str[SB_BUFFER_SIZE]; // sensor data string

  char sb_buffer[SB_BUFFER_SIZE]; // software bus buffer
  //int ret;

  // period should be 'ms' unit
  while(keep_running){
    {
    std::lock_guard<std::mutex> lock(i2c_mutex);
    sensor.read_data();
    } // mutex lock scope.. scope 나가면 자동 unlock
    data = sensor.get_data();
    
    // for(int i=0; i<data.size(); i++){
    //   std::cout << data[i] << " ";
    // }

    // convert data to string and set message data
    auto now_time = std::chrono::system_clock::now();
    auto now_ms = time_point_cast<milliseconds>(now_time);

    auto ret = fmt::format_to_n(data_str, SB_BUFFER_SIZE -1, "{:%H:%M:%S},{}", now_ms fmt::join(data, ","));
    *ret.out = '\0';
    
    fmt::print("SENSOR APP RUNNING at {:%Y-%m-%d %H:%M:%S}\n", now_time);
  

    msg.set_data(data_str);
    {
    std::lock_guard<std::mutex> lock(sb_mutex);
    write(sockfd, msg.get_message(), SB_BUFFER_SIZE); // send to software bus
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(period));
  }
}
