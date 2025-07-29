#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <vector>

#include <cstring>     // memset()
#include <unistd.h>    // close()
#include <cstdlib>    // exit()

#include <queue>
#include <thread>
#include <mutex>
#include <map>
#include <string>

#include "connection.h" // SOCKET_NAME

// 인자로 software bus에 연결할 app의 개수를 받음
int main() {
  int app_num = APP_NUM; // 앱의 개수, connection.h에서 정의된 상수 사용
  std::vector<int> app_sockets(app_num); // 각 앱과의 소켓을 저장할 벡터
  std::vector<std::string> app_names = {"SENSOR", "COMM", "CONTROL"}; // 앱 이름 벡터, index는 app_id와 동일
  std::vector<std::string> message_names = {"DATA", "IMU", "BARO", "COMMAND", "ERROR"}; // 메시지 이름 벡터, index는 message_id와 동일


  /*/////////////////////////////////////////////////////////////
  Socket Hosting
  */////////////////////////////////////////////////////////////
  int ret; // return value
  int down_flag = 0;
  int sb_host_socket; // 소켓 파일 디스크립터
  int result;
  int epoll_fd; // epoll file descriptor
  int nready; // number of ready events
  struct sockaddr_un name; // 소켓 이름 구조체
  struct epoll_event ev, events[MAX_EVENTS]; // epoll 이벤트 구조체
  char buffer[SB_BUFFER_SIZE]; // 데이터 전송 버퍼
  int buffer_app_id;
  int buffer_message_id;
  
  unlink(SOCKET_NAME); // 소켓 이름이 중복되면 안되니까 미리 삭제

  sb_host_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (sb_host_socket == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /*
  * For portability clear the whole structure, since some
  * implementations have additional (nonstandard) fields in
  * the structure.
  */
  memset(&name, 0, sizeof(name));

  /* Bind socket to socket name. */

  name.sun_family = AF_UNIX;
  strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

  ret = bind(sb_host_socket, (const struct sockaddr *) &name, sizeof(name));
  if (ret == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  /*
  * Prepare for accepting connections. The backlog size is set
  * to 20. So while one request is being processed other requests
  * can be waiting.
  */

  ret = listen(sb_host_socket, app_num); // 연결 대기 함수.. app_num 만큼의 연결을 대기
  if (ret == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  // epoll 생성 및 리스닝 소켓 등록
  epoll_fd = epoll_create1(0);
  ev.events = EPOLLIN;         // 읽기 이벤트 감시
  ev.data.fd = sb_host_socket;      // 감시 대상: 리스닝 소켓
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sb_host_socket, &ev);

  int sensor_counter[3] = {0, 0, 0};

  while(1) {
    nready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

    for (int i = 0; i < nready; i++) {
      if (events[i].data.fd == sb_host_socket) {
          // 새 클라이언트 연결 요청이 들어온 경우 (복구 대비용 코드)
          int client_fd = accept(sb_host_socket, NULL, NULL);
            
          // 새 클라이언트 소켓을 epoll 감시 대상에 등록
          ev.events = EPOLLIN;
          ev.data.fd = client_fd;
          epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
      } 
      else {
        // 기존 클라이언트 소켓에서 데이터 도착 시 처리
        int fd = events[i].data.fd;
        int n = recv(fd, buffer, sizeof(buffer), 0);
        if (n <= 0) {
          // 클라이언트 종료: epoll에서 제거 및 소켓 닫기
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
          close(fd);
        }
        else {
          // 받은 데이터 에코
          buffer[n] = '\0';
          buffer_app_id = buffer[0] - '0'; // 첫 번째 문자를 앱 ID로 변환
          buffer_message_id = buffer[2] - '0'; // 세 번째 문자를 메시지 ID로 변환
          sensor_counter[buffer_message_id]++;
          printf("%d, %d, %d\n", sensor_counter[0], sensor_counter[1], sensor_counter[2]);
          printf("[%s][%s] : %s\n", app_names[buffer_app_id].data(), message_names[buffer_message_id].data(), buffer);
        }
      }
    }
  }
  ///////////////////////////////////////////////////////////////////////////////
}
