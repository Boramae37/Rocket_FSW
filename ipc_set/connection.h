#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>

#ifndef CONNECTION_H
#define CONNECTION_H


#define SOCKET_NAME "/tmp/9Lq7BNBnBycd6nxy.socket"
#define SB_BUFFER_SIZE 1024 // socket message size, 센서, 특히 imu 센서 길이 잘 확인해서 결정할 것

#define MAX_EVENTS 10

// App ID
constexpr int APP_NUM = 3; // 현재 3개 앱
constexpr int SENSOR_ID = 0;
constexpr int COMM_ID = 1;
constexpr int CONTROL_ID = 2;
 
// Message ID
constexpr int IMU_ID = 0;              // IMU 센서 id
constexpr int BARO_ID = 1;             // Barometer 센서 id
constexpr int GPS_ID = 2;              // GPS 센서 id
constexpr int COMMAND_MESSAGEID = 3;   // 명령 메시지
constexpr int ERROR_MESSAGEID = 4;     // 에러 메시지
constexpr int PARACHUTE_MESSAGEID = 5; // 낙하산 메시지
constexpr int SET_FD_COMMAND = 6;     // 데이터 메시지

// Command ID (message 부분에 들어감. 맨 처음. 즉 buffer[5]에 들어감) 
constexpr int STOP_COMMAND = 0; // stop command
constexpr int START_COMMAND = 1; // start command

constexpr char PARACHUTE_STATUS[] = "1"; // 메시지 내용

// Communication App 주기 (몇 회마다 보낼지)
constexpr int COMM_PERIOD_IMU = 20;
constexpr int COMM_PERIOD_BARO = 20;
constexpr int COMM_PERIOD_GPS = 20;

// Subscribe Map
extern std::unordered_map<int, std::vector<int>> subscribe_map;

class Message{
  private:
    int app_id; // 메시지 보낸 앱의 id
    int message_id; // 메시지 받을 앱의 id
    char message[SB_BUFFER_SIZE-5]; // 메시지 내용. id 가 추가될 것이니 넉넉히 5바이트 뺌
    char buffer[SB_BUFFER_SIZE]; // 데이터 전송 버퍼

  public:
    Message(int);
    ~Message();
    char* get_message(); // cstyle로 데이터 반환(for socket ipc)
    void set_message_id(int);
    void set_data(char* data);
};


#endif  // include guard

