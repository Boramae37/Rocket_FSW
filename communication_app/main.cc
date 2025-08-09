#include <iostream>
#include <sys/socket.h> // socket(), connect(), write()
#include <sys/un.h>    // struct sockaddr_un
#include <cstdlib>     // exit()
#include <cstdio>      // snprintf()
#include <charconv>   // std::to_chars
#include <thread>
#include <atomic>      // std::atomic

#include <termios.h>   // termios, tcgetattr(), cfsetispeed(), cfsetospeed(), tcsetattr()
#include <fcntl.h>      // open(), O_RDWR
#include <unistd.h>     // read(), write(), close(), usleep()

#include <sys/epoll.h>      // epoll_create1(), epoll_ctl(), epoll_wait()
#include <cstring>      // memset(), strerror()

#include "spdlog/fmt/fmt.h" // fmt library for formatting strings


#include "connection.h" // for communication with software bus


constexpr char SERIAL_PORT[] = "/dev/ttyUSB0"; // UART 포트 이름
constexpr int BAUD_RATE = B115200; // UART 통신 속도
constexpr int UART_BUFFER_SIZE = SB_BUFFER_SIZE; // 소프트웨어 버스 메시지 크기

std::atomic<bool> keep_running(true); // 프로그램 종료 플래그

int setup_uart(const char* device);

int main(){
  fmt::print("this is communication_application\n");

  //////////////////////////////////////////////////////////////////////////
  // Software Bus 연결을 위한 공통 부분(물론 SENSOR_ID등 등록 부분은 바꿔야함)
  //////////////////////////////////////////////////////////////////////////

  // Socket setup
  int ret; // return value
  int sockfd; // socket file descriptor
  struct sockaddr_un addr;
  char sb_buffer[SB_BUFFER_SIZE];
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
  Message msg(COMM_ID);
  msg.set_message_id(SET_FD_COMMAND);
  snprintf(sb_buffer, sizeof(sb_buffer), "%d", sockfd);
  msg.set_data(sb_buffer);
  write(sockfd, msg.get_message(), SB_BUFFER_SIZE);

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


  // uart setup
  int uart_fd = setup_uart(SERIAL_PORT);
  if (uart_fd == -1) {
    perror("setup_uart");
    exit(EXIT_FAILURE);
  }
  char uart_buffer[UART_BUFFER_SIZE]; // UART로 읽어올 데이터 버퍼
  char uart_temp_buffer[UART_BUFFER_SIZE]; // UART로 읽어올 데이터 버퍼
  int uart_buffer_idx = 0; // UART 버퍼 인덱스

//////////////////////////////////////////////////////////////////////
// EPOLL setup for listening to UART COMMANDs
///////////////////////////////////////////////////////////////////
  int epoll_fd; // epoll file descriptor
  int nready; // number of ready events
  int temp_fd;
  struct epoll_event ev, events[1]; // epoll 이벤트 구조체

  // epoll 생성 및 리스닝 소켓 등록(UART)
  epoll_fd = epoll_create1(0);
  ev.events = EPOLLIN;         // 읽기 이벤트 감시
  ev.data.fd = uart_fd;      // 감시 대상: 리스닝 소켓
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, uart_fd, &ev);

  // SB 용 소켓 등록
  ev.events = EPOLLIN;         // 읽기 이벤트 감시
  ev.data.fd = sockfd;      // 감시 대상: 리스닝 소켓
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev);

////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
  std::thread listen_thread([&](){
    while(keep_running) {
      nready = epoll_wait(epoll_fd, events, 1, -1); // -1은 타임아웃(즉, 무한대기)
      memset(uart_temp_buffer, 0, sizeof(uart_temp_buffer));
      for (int i = 0; i < nready; i++) {
        temp_fd = events[i].data.fd;
        // 지상국 명령 수신
        if (temp_fd == uart_fd) {
          ret = read(uart_fd, uart_temp_buffer, UART_BUFFER_SIZE); // UART로부터 데이터 읽기(1바이트씩 읽어야 함)
          uart_temp_buffer[ret] = '\0';
          ret = strlen(uart_temp_buffer);
          uart_temp_buffer[ret] = '\0'; // 문자열 끝에 NULL 추가
          //fmt::print("ret:{}, tmep: {}", ret, uart_temp_buffer);
          for (int j=0; j<ret; j++){
            strcpy(&uart_buffer[uart_buffer_idx], &uart_temp_buffer[j]);
            uart_buffer_idx++;
            //fmt::print("j : {}\n", j);
            //fmt::print("buffer : {}\n", uart_buffer);

            // end of message detection
            if(uart_temp_buffer[j] - '\0' == TELE_END) {
              fmt::print("end of str detected\n");
              if(uart_buffer[0] -'0' == COMMAND_MESSAGEID) {
                int command_id = uart_buffer[2] - '0'; // 명령 ID는 3번째 문자에 위치

                // stop command
                if (command_id == STOP_COMMAND){
                  snprintf(sb_buffer, SB_BUFFER_SIZE, "%d,%d,%d\n", COMM_ID, COMMAND_MESSAGEID, STOP_COMMAND);
                  write(sockfd, sb_buffer, SB_BUFFER_SIZE); // 소프트웨어 버스로 데이터 전송
                  fmt::print("[COMM][UART] STOP COMMAND RECEIVED : {}\n", uart_buffer);
                  
                }
                // start command
                else if (command_id == START_COMMAND){
                  snprintf(sb_buffer, SB_BUFFER_SIZE, "%d,%d,%d\n", COMM_ID, COMMAND_MESSAGEID, START_COMMAND);
                  write(sockfd, sb_buffer, SB_BUFFER_SIZE); // 소프트웨어 버스로 데이터 전송
                  fmt::print("[COMM][UART] START COMMAND RECEIVED : {}\n", uart_buffer);
                }
                // echo
                else{
                  ret = strlen(uart_buffer);
                  write(uart_fd, uart_buffer,ret);
                }
              }
              
              fmt::print("[COMM][UART] Received: {}\n", uart_buffer); // UART로부터 받은 데이터 출력
              auto result = fmt::format_to_n(sb_buffer, SB_BUFFER_SIZE - 1, "{},{},{} : received", COMM_ID, INFO_ID, uart_buffer);
              *result.out = '\0';
              write(uart_fd, sb_buffer, result.size);
              memset(uart_buffer, 0, sizeof(uart_buffer)); // 버퍼 초기화
              uart_buffer_idx = 0; // 포인터 초기화
            }
          }
        }
        // 내부 소프트웨어 버스 메세지 수신
        else if (temp_fd == sockfd) {
          // 소프트웨어 버스로부터 데이터 읽기
          read(sockfd, sb_buffer, SB_BUFFER_SIZE);
          ret = strlen(sb_buffer);
          sb_buffer[ret] = '\0'; // 문자열 끝에 NULL 추가
          // printf("ret = %d\n", ret);
          // fmt::print("COMM received {}\n", sb_buffer);
          write(uart_fd, sb_buffer, ret); // 읽은 데이터를 UART로 전송
          // printf("[COMM][SB] Translating %d bytes : %s\n", ret, sb_buffer); // 소프트웨어 버스로부터 받은 데이터 출력
          if (sb_buffer[0]-'0' == SB_ID && sb_buffer[2] - '0' == COMMAND_MESSAGEID && sb_buffer[4] - '0' == STOP_COMMAND) {
            fmt::print("[COMM] : STOP COMMAND RECEIVED\n");
            
            keep_running = false;
            break;
          }
        }
      }
    }
  });
  listen_thread.join();
  fmt::print("COMM die\n");
}

int setup_uart(const char* device) {
    // UART 포트 오픈 (읽기/쓰기, 터미널이 아님, 논블로킹)
    int fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1) return -1;  // 오픈 실패 시 -1 반환

    struct termios options{};                   // 시리얼 설정 구조체 초기화
    tcgetattr(fd, &options);                    // 현재 포트 설정 읽어옴

    // option들을 xbee 설정(xctu 설정)과 맞춰야함.
    cfsetispeed(&options, BAUD_RATE);           // 입력 속도 설정
    cfsetospeed(&options, BAUD_RATE);           // 출력 속도 설정
    
    
    options.c_cflag |= (CLOCAL | CREAD);        // UART 리시버/로컬 사용 허가
    options.c_cflag &= ~CSIZE;                  // 데이터 비트 수 마스킹
    options.c_cflag |= CS8;                     // 데이터 비트 8비트
    options.c_cflag &= ~PARENB;                 // 패리티 비트 사용 안함
    options.c_cflag &= ~CSTOPB;                 // 스톱 비트 1비트
    options.c_cflag &= ~CRTSCTS;                // 하드웨어 플로우 제어 안함

    // canonical 모드해제, echo, echoe, isig 플래그 해제
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG); // 원시 입력 모드로 설정
    options.c_iflag &= ~(IXON | IXOFF | IXANY);         // 소프트웨어 플로우 제어 해제
    options.c_oflag &= ~OPOST;                          // 출력 후처리 해제

    tcsetattr(fd, TCSANOW, &options);           // 설정 즉시 적용
    return fd;                                  // UART 디스크립터 반환
}
