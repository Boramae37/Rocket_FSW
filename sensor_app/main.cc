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


// my includes
#include "sensor.h"
#include "connection.h" // for communication with software bus

// function declarations
std::string data_to_string(const std::vector<float>& data, std::string& str);
void thread_read_sensor(Sensor& sensor, int period, int sensor_id);



// global variables
std::mutex i2c_mutex; // i2c bus mutex
std::mutex sb_mutex; // software bus mutex
std::atomic<bool> keep_running(true);



///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////



int sockfd; // socket file descriptor
int main(){

  // Socket setup
  int ret; // return value
  
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

  

  // Sensor setup
  BMP390 baro;
  BNO055 imu;

  std::vector<float> baro_data;
  std::vector<float> imu_data;
  

  baro.init();
  usleep(1000000); // 1 sec
  imu.init();
  usleep(1000000); // 1 sec
  

  /*
  for(int i=0; i<1000; i++){
    baro.read_data();
    baro_data = baro.get_data();
    std::cout << "Barometer Data: Pressure = " << baro_data[0] 
              << ", Temperature = " << baro_data[1] << std::endl;
    usleep(20000);
    imu.read_data();
    usleep(20000);
    imu_data = imu.get_data();
    std::cout << "IMU Data: Acceleration = (" << imu_data[0] << ", " << imu_data[1] << ", " << imu_data[2] << "), "
              << "Gyroscope = (" << imu_data[3] << ", " << imu_data[4] << ", " << imu_data[5] << "), "
              << "Euler Angles = (" << imu_data[6] << ", " << imu_data[7] << ", " << imu_data[8] << "), "
              << "Quaternion = (" << imu_data[9] << ", " << imu_data[10] << ", " << imu_data[11] << ", " << imu_data[12] << "), "
              << "Linear Acceleration = (" << imu_data[13] << ", " << imu_data[14] << ", " << imu_data[15] << ")" 
              << std::endl;
    usleep(20000);
  }
  */

  
  // Sensor threads
  // 센서 ID는 "connection.h"에서 정의된 상수 사용
  std::thread barometer_thread(
    [&] { thread_read_sensor (std::ref(baro), 10, BARO_ID); }
  );

  std::thread imu_thread(
    [&] { thread_read_sensor(std::ref(imu), 12, IMU_ID); }
  );




  std::this_thread::sleep_for(std::chrono::seconds(100)); // 5초 후 종료
  keep_running = false;
  
  //barometer_thread.join();
  imu_thread.join();

  return 0;
}



bool data_to_string(const std::vector<float>& values, char* out_buf) {
  char* ptr = out_buf;
  char* buf_end = out_buf + SB_BUFFER_SIZE;

  for (size_t i = 0; i < values.size(); ++i) {
      char num_buf[32];
      auto [num_end, ec] = std::to_chars(num_buf, num_buf + sizeof(num_buf), values[i]);
      if (ec != std::errc()) return false;

      size_t len = num_end - num_buf;
      if (ptr + len >= buf_end) return false;

      std::memcpy(ptr, num_buf, len);
      ptr += len;

      if (i + 1 < values.size()) {
          if (ptr + 1 >= buf_end) return false;
          *ptr++ = ',';
      }
  }

  if (ptr >= buf_end) return false;
  *ptr = '\0';
  return true;
}


void thread_read_sensor(Sensor& sensor, int period, int sensor_id){
  std::vector<float> data;
  Message msg(SENSOR_ID); // sensor_id is the specific sensor id (IMU or Barometer)
  msg.set_message_id(sensor_id);
  char data_str[SB_BUFFER_SIZE]; // sensor data string

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
    data_to_string(data, data_str);
    msg.set_data(data_str);
    {
    std::lock_guard<std::mutex> lock(sb_mutex);
    write(sockfd, msg.get_message(), SB_BUFFER_SIZE);
    }
    
    std::cout << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(period));
  }
}
