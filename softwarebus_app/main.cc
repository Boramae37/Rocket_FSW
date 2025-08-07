#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <vector>
#include <filesystem>

#include <cstring>     // memset()
#include <unistd.h>    // close()
#include <cstdlib>    // exit()


#include <thread>
#include <mutex>
#include <map>
#include <string>

#include "spdlog/async.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "spdlog/fmt/fmt.h" // spdlog에 내장된 fmt 헤더를 포함

#include <csignal> // signal handling

#include "connection.h" // SOCKET_NAME

#define DEBUG

// 시그널 핸들러에서 전역 로거에 접근할 수 있도록 전역 네임스페이스에 선언
namespace SB {
    std::shared_ptr<spdlog::logger> logger;
}


// Signal handler for graceful shutdown
void signal_handler(int signal) {
    std::cout << "\n종료 신호 (" << signal << ") 수신! 모든 로그를 안전하게 저장합니다..." << std::endl;
    
    // spdlog의 모든 자원을 정리하고, 큐에 남아있는 로그를 파일에 강제로 쓰게 합니다.
    spdlog::shutdown();
    
    // 프로그램을 정상 종료시킵니다.
    exit(signal);
}




// 인자로 software bus에 연결할 app의 개수를 받음
int main() {
/////////////////////////////////////////////////////////////////////////////////
//       Logger setup
////////////////////////////////////////////////////////////////////////////////
  // 1. 시그널 핸들러 등록: Ctrl+C (SIGINT) 와 kill (SIGTERM) 신호 처리
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);


  // 2. 비동기 로깅을 위한 스레드 풀 초기화
  // 큐(버퍼) 크기는 8192개, 백그라운드 스레드는 1개로 설정
  spdlog::init_thread_pool(8192, 1);

  // 3. 사용할 싱크(출력 대상)들을 담을 벡터 생성
  std::vector<spdlog::sink_ptr> sinks;

  // 4. 조건부 콘솔 싱크 추가 (디버그 모드에서만 활성화)
  #ifdef DEBUG
    // 컬러 콘솔 싱크를 생성하여 sinks 벡터에 추가
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    fmt::print("[알림] 디버그 모드로 실행되어 콘솔 출력이 활성화되었습니다.\n");
  #endif

  // 5. 파일 싱크 추가 (항상 활성화)
  // "logs/app_log.txt" 파일에 로그를 기록하고, 기존 내용에 이어붙입니다.
  //std::filesystem::create_directories("logs/");
  sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("../logs/app_log.txt", true));

  // 6. 비동기(async) 로거 생성
  // 위에서 만든 싱크 목록을 사용하여 "main_logger"라는 이름의 비동기 로거를 만듭니다.
  SB::logger = std::make_shared<spdlog::async_logger>("main_logger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
  
  // 7. 로거를 전역으로 등록하여 spdlog::info(...) 형태로 쉽게 사용
  spdlog::register_logger(SB::logger);
  
  // 로깅 레벨 설정
  SB::logger->set_level(spdlog::level::trace); // 모든 레벨의 로그를 기록
  // 특정 레벨 이상의 로그가 발생하면 즉시 파일에 쓰도록 설정 (추가적인 안전장치)
  SB::logger->flush_on(spdlog::level::critical);

  // 8. 프로그램 메인 로직
  SB::logger->info("========================================");
  SB::logger->info("애플리케이션 시작 (PID: {})", getpid());
  SB::logger->info("========================================");
  
  #ifndef DEBUG
    fmt::print("[알림] 릴리즈 모드로 실행 중입니다. 로그는 'logs/app_log.txt' 파일에만 기록됩니다.\n");
  #endif
  fmt::print("프로그램 실행 중... (Ctrl+C를 눌러 종료하세요)\n");

/////////////////////////////////////////////////////////////////////////////////


  int app_num = APP_NUM; // 앱의 개수, connection.h에서 정의된 상수 사용

  // 각 app의 fd 저장
  std::vector<int> app_fd_list(app_num);
  app_fd_list[SENSOR_ID] = -1; // 센서 앱은 소켓을 열지 않음
  app_fd_list[COMM_ID] = -1;   // 커뮤니케이션 앱은 소켓을 열지 않음
  app_fd_list[CONTROL_ID] = -1; // 컨트롤 앱은 소켓을 열지 않음

  // 앱 이름 벡터, index는 app_id와 동일
  std::vector<std::string> app_names = {"SENSOR", "COMM", "CONTROL"}; 

  // 메시지 이름 벡터, index는 message_id와 동일
  std::vector<std::string> message_names = {"IMU", "BARO", "GPS", "COMMAND", "ERROR", "PARACHUTE", "SET_FD_COMMAND"};
  
  // 앱 ID와 연결된 소켓 FD를 저장하는 이중리스트(맵을 바꿈. 맵이 수정에는 더 편함, 속도에선 느림)
  std::vector<std::vector<int>> app_subscribers(APP_NUM);
  app_subscribers[SENSOR_ID] = subscribe_map[SENSOR_ID];
  app_subscribers[COMM_ID] = subscribe_map[COMM_ID];
  app_subscribers[CONTROL_ID] = subscribe_map[CONTROL_ID];


  /*///////////////////////////////////////////////////////
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

  int n; // number of bytes read
  int fd; // epoll에서 받은 파일 디스크립터

  
  
  unlink(SOCKET_NAME); // 소켓 이름이 중복되면 안되니까 미리 삭제

  sb_host_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (sb_host_socket == -1) {
    SB::logger->error("socket not created: {}", strerror(errno));
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
    SB::logger->error("bind failed: {}", strerror(errno));
    exit(EXIT_FAILURE);
  }

  /*
  * Prepare for accepting connections. The backlog size is set
  * to 20. So while one request is being processed other requests
  * can be waiting.
  */

  ret = listen(sb_host_socket, app_num); // 연결 대기 함수.. app_num 만큼의 연결을 대기
  if (ret == -1) {
    SB::logger->error("listen failed: {}", strerror(errno));
    exit(EXIT_FAILURE);
  }

  SB::logger->info("소프트웨어 버스 소켓이 {}로 시작되었습니다.", SOCKET_NAME);

  // epoll 생성 및 리스닝 소켓 등록
  epoll_fd = epoll_create1(0);
  ev.events = EPOLLIN;         // 읽기 이벤트 감시
  ev.data.fd = sb_host_socket;      // 감시 대상: 리스닝 소켓
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sb_host_socket, &ev);

  int sensor_counter[8] = { 0 };

  // 각 imu, baro, gps 센서값 개수 카운터
  // 인덱스는 connection.h에서 정의된 메시지 ID와 동일
  int comm_counter[3] = { 0 };
  int comm_period[3] = { COMM_PERIOD_IMU, COMM_PERIOD_BARO, COMM_PERIOD_GPS };

  while(1) {
    nready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

    for (int i = 0; i < nready; i++) {
      fd = events[i].data.fd;
      if (fd == sb_host_socket) {
        // 새 클라이언트 연결 요청이 들어온 경우 events[i].data.fd는 sb_host_socket
        // 이미 accept 된 클라이언트인 경우 해당 클라이언트에 대응되는 fd가 들어옴
        int client_fd = accept(sb_host_socket, NULL, NULL);
        
        // 새 클라이언트 소켓을 epoll 감시 대상에 등록
        ev.events = EPOLLIN;
        ev.data.fd = client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
      }
      else {
        // 기존 클라이언트 소켓에서 데이터 도착 시 처리
        n = recv(fd, buffer, sizeof(buffer), 0);
        if (n <= 0) {
          // 클라이언트 종료: epoll에서 제거 및 소켓 닫기
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
          close(fd);
        }
        else {
          if (buffer[2] - '0' == SET_FD_COMMAND){
            app_fd_list[buffer[0] - '0'] = fd; // fd를 앱 ID에 매핑
            SB::logger->info("App {} connected with fd {}", app_names[buffer[0] - '0'].data(), fd);
          }
          else {
            // 데이터 처리
            buffer[n] = '\0';
            buffer_app_id = buffer[0] - '0'; // 첫 번째 문자를 앱 ID로 변환
            buffer_message_id = buffer[2] - '0'; // 세 번째 문자를 메시지 ID로 변환
            

            // 구독자들에게 공유
            for (int subscriber_id : app_subscribers[buffer_app_id]) {
              SB::logger->info("[{}] received send to [{}]", app_names[buffer_app_id].data(), app_names[subscriber_id].data());
              if (app_fd_list[subscriber_id] != -1) { // 해당 앱이 연결되어 있는지 확인
                // communication_app 통신 주기에 맞춰 일단 SB에서 주기에 맞춰 준다.
                // 주기는 시간이 아닌 counter(횟수)로 관리
                if (buffer_app_id == SENSOR_ID && subscriber_id == COMM_ID) {
                  comm_counter[buffer_message_id]++;

                  if (comm_counter[buffer_app_id] == comm_period[buffer_message_id]) {
                    comm_counter[buffer_app_id] = 0;
                    SB::logger->info("Sending to COMM app: {}", buffer);
                    send(app_fd_list[subscriber_id], buffer, n, 0); // 구독
                  }
                }
                else{
                  send(app_fd_list[subscriber_id], buffer, n, 0); // 구독자에게 데이터 전송
                }
              }
            }
            // 받은 데이터 에코
            sensor_counter[buffer_message_id]++;
            SB::logger->info("{} {} {}", sensor_counter[0], sensor_counter[1], sensor_counter[2]);
            SB::logger->info("[{}][{}] : {}", app_names[buffer_app_id].data(), message_names[buffer_message_id].data(), buffer);
          }
        }
      }
    }
  }
  ///////////////////////////////////////////////////////////////////////////////
}
