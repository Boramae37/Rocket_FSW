#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <sys/socket.h> // socket(), connect(), write()
#include <sys/un.h>    // struct sockaddr_un
#include <sys/epoll.h> // epoll functions
#include <stdlib.h>     // atoi, atof
#include <unistd.h>     // close(), usleep()

#include "connection.h"



int main(){
  std::cout << "this is control_application" << std::endl;

  // message class
  Message msg(CONTROL_ID);
  msg.set_message_id(COMMAND_MESSAGEID);
  char message_buf[SB_BUFFER_SIZE];

  // Socket setup
  int ret; // return value
  int sockfd; // socket file descriptor
  int nready; // number of ready events
  struct sockaddr_un addr;
  struct epoll_event ev, events[MAX_EVENTS]; // epoll 이벤트 구조체

  char buffer[SB_BUFFER_SIZE];
  char* token; // buffer에서 원하는 토큰을 추출하기 위한 포인터

  int n; // number of bytes read
  int fd; // epoll에서 받은 파일 디스크립터
  
  float temp_barometer = 0;
  float max_barometer = 0;
  int parachute_counter = 0;
  bool parachute_deployed = false;
  
  sockfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);
  connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));

  int epfd = epoll_create(MAX_EVENTS);

  
  ev.events = EPOLLIN;
  ev.data.fd = sockfd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

  
  while (!parachute_deployed) {
    nready = epoll_wait(epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < nready; i++) {
      fd = events[i].data.fd;
      if (fd != sockfd) {
        // 데이터가 도착함 → read/recv 사용
        n = recv(fd, buffer, sizeof(buffer), 0);
        if (n > 0) {
          // 낙하산 전개 조건
          // barometer 데이터 인지 확인 ... 근데 이거 software bus에서 해도 될듯?
          if (buffer[2] - '0' == BARO_ID){
            // 첫 번째 토큰
            token = strtok(buffer, ",");
            // 두 번째 토큰
            token = strtok(NULL, ",");
            // 세 번째 토큰
            token = strtok(NULL, ",");
            temp_barometer = atof(token);
            if (max_barometer < temp_barometer) {
              max_barometer = temp_barometer;
            }
            else if(max_barometer - temp_barometer > 0.01){ // m 단위
              parachute_counter++;
              if (parachute_counter > 5) {
                parachute_deployed = true;
                printf("Parachute Deployed\n");
                strcpy(message_buf, PARACHUTE_STATUS);
                msg.set_message_id(PARACHUTE_MESSAGEID);
                msg.set_data(message_buf);
                write(sockfd, msg.get_message(), SB_BUFFER_SIZE);
              }
            }
          }
          buffer[n] = '\0';
          std::cout << "[Control_APP Received]: " << buffer << std::endl;
        }
      }
    }
  }
}
