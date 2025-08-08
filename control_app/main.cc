#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <sys/socket.h> // socket(), connect(), write()
#include <sys/un.h>    // struct sockaddr_un
#include <sys/epoll.h> // epoll functions
#include <stdlib.h>     // atoi, atof
#include <unistd.h>     // close(), usleep()

#include <pigpiod_if2.h>
#include <chrono>
#include <thread>

#include "spdlog/fmt/fmt.h"
#include "connection.h"

// 사용할 GPIO 핀 (하드웨어 PWM 지원 핀)
const int PWM_PIN = 12;



int main(){

  int pi = pigpio_start(NULL, NULL); // localhost:8888
  set_servo_pulsewidth(pi, PWM_PIN, 1500); // 고정
  std::this_thread::sleep_for(std::chrono::seconds(1));
  

  
  std::cout << "this is control_application" << std::endl;
  


  bool parachute_deployed = false;
  int parachute_counter = 0;
  int temp_barometer = 0;
  int max_barometer = 0;

  char *token;

  // message class
  
  ;
  char message_buf[SB_BUFFER_SIZE];

  //////////////////////////////////////////////////////////////////////////
  // Software Bus 연결을 위한 공통 부분(물론 SENSOR_ID등 등록 부분은 바꿔야함)
  //////////////////////////////////////////////////////////////////////////

  // Socket setup
  int ret; // return value
  int sockfd; // socket file descriptor
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
  Message msg(CONTROL_ID);
  msg.set_message_id(SET_FD_COMMAND);
  snprintf(buffer, sizeof(buffer), "%d", sockfd);
  msg.set_data(buffer);
  write(sockfd, msg.get_message(), SB_BUFFER_SIZE);
  msg.set_message_id(COMMAND_MESSAGEID); 
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// EPOLL setup for listening to UART COMMANDs
///////////////////////////////////////////////////////////////////
  int epoll_fd; // epoll file descriptor
  int nready; // number of ready events
  int temp_fd;
  struct epoll_event ev, events[10]; // epoll 이벤트 구조체

  
  // SB 용 소켓 등록
  epoll_fd = epoll_create1(0);
  ev.events = EPOLLIN;         // 읽기 이벤트 감시
  ev.data.fd = sockfd;      // 감시 대상: 리스닝 소켓
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev);

////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

  

  while (!parachute_deployed) {
    nready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    for (int i = 0; i < nready; i++) {
      temp_fd = events[i].data.fd;
      if (temp_fd != sockfd) {
        // 데이터가 도착함 → read/recv 사용
        ret = recv(temp_fd, buffer, sizeof(buffer), 0);
        buffer[ret] = '\0';
        printf("CONTROL: RECIEVED : %s", buffer);
        if (ret > 0) {
          // 낙하산 전개 조건
          // barometer 데이터 인지 확인 ... 근데 이거 software bus에서 해도 될듯?
          if (buffer[2] - '0' == BARO_ID){
            // 첫 번째 토큰
            token = strtok(buffer, ",");
            // 두 번째 토큰
            token = strtok(NULL, ",");
            // 세 번째 토큰
            token = strtok(NULL, ",");
            // 네 번쨰 토큰 = 고도
            token = strtok(NULL, ","); 
            temp_barometer = atof(token);
            printf("CONTROL : RECEIVED : %f", temp_barometer);
            if (max_barometer < temp_barometer) {
              max_barometer = temp_barometer;
            }
            else if(max_barometer - temp_barometer > 0.1){ // m 단위
              parachute_counter++;
              if (parachute_counter > 5) {
                parachute_deployed = true;
                set_servo_pulsewidth(pi, PWM_PIN, 500); // 해제 
                printf("Parachute Deployed\n");
                strcpy(message_buf, PARACHUTE_STATUS);
                msg.set_message_id(PARACHUTE_MESSAGEID);
                msg.set_data(message_buf);
                write(sockfd, msg.get_message(), SB_BUFFER_SIZE);
              }
            }

          }
          std::cout << "[Control_APP Received]: " << buffer << std::endl;
        }
      }
    }
  }
}
