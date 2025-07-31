#include <iostream>
#include <unistd.h>
#include <sys/types.h> // pid_t
#include <sys/wait.h> // waitpid()
#include <cstdlib>  // exit()

#define softwarebus_app 0
#define sensor_app 1
#define com_app 2
#define ctrl_app 3

int main() {
  
  pid_t pid[4];

  // fork()를 만드는 순간, 부모와 자식 프로세스가 복제되고, 각각의 pid[softwarebus_app]에 숫자가 반환된다. 부모에는 자식의 pid, 자식에게는 0
  pid[softwarebus_app] = fork(); 
  if(pid[softwarebus_app] == 0) {
    // 자식 프로세스에서 softwarebus_app 실행
    std::cout << "PID["<< getpid() <<"] Launch Software Bus Application" << std::endl;
    execl("../../run/softwarebus", "softwarebus", NULL);
    std::cerr << "Failed to launch Software Bus Application" << std::endl;
    exit(1);
  } else if (pid[softwarebus_app] < 0) {
    std::cerr << "Failed to fork Software Bus Application" << std::endl;
    exit(1);
  }
  else{
    usleep(500000); // sb_app이 완전히 시작될 때까지 기다리기 위함
  }

  pid[sensor_app] = fork();

  // 0이면 자식이란 말이기에..
  if(pid[sensor_app] == 0){

    std::cout << "PID["<< getpid() <<"] Launch Sensor Application" << std::endl;
    // 새로운 프로그램으로 덮어 씌운다.
    execl("../../run/sensor", "sensor", NULL);
    std::cout << "Failed to launch Sensor Application" << std::endl;
    exit(1);
  }
  else if (pid[sensor_app] < 0) {
    std::cerr << "Failed to fork Sensor Application" << std::endl;
    exit(1);
  }
  else {
    int sensor_status;
    //waitpid(pid[sensor_app], &sensor_status, 0);
    //std::cout << "[supervisor] : Sensor App terminated" << std::endl;
  }

  pid[com_app] = fork();

  // 0이면 자식이란 말이기에..
  if(pid[com_app] == 0){

    std::cout << "PID["<< getpid() <<"] Launch Communication Application" << std::endl;
    // 새로운 프로그램으로 덮어 씌운다.
    execl("../../run/communication", "communication", NULL);
    std::cout << "Failed to launch Communication Application" << std::endl;
    exit(1);
  }
  else if (pid[com_app] < 0) {
    std::cerr << "Failed to fork Communication Application" << std::endl;
    exit(1);
  }
  else {
    int com_status;
    //waitpid(pid[com_app], &com_status, 0);
    //std::cout << "[supervisor] : Communication App terminated" << std::endl;
  }


  pid[ctrl_app] = fork();

  // 0이면 자식이란 말이기에..
  if(pid[ctrl_app] == 0){

    std::cout << "PID["<< getpid() <<"] Launch Control Application" << std::endl;
    // 새로운 프로그램으로 덮어 씌운다.
    execl("../../run/control", "control", NULL);
    std::cout << "Failed to launch Control Application" << std::endl;
    exit(1);
  }
  else if (pid[ctrl_app] < 0) {
    std::cerr << "Failed to fork Control Application" << std::endl;
    exit(1);
  }
  else {
    int ctrl_status;
    //waitpid(pid[ctrl_app], &ctrl_status, 0);
    //std::cout << "[supervisor] : Control App terminated" << std::endl;
  }
  
  int softwarebus_status;
  int ctrl_status;
  int com_status;    
  int sensor_status;

  waitpid(pid[sensor_app], &sensor_status, 0);
  waitpid(pid[com_app], &com_status, 0);
  waitpid(pid[ctrl_app], &ctrl_status, 0);
  waitpid(pid[softwarebus_app], &softwarebus_status, 0);
  std::cout << "[supervisor] : Sensor App terminated" << std::endl;
  std::cout << "[supervisor] : Communication App terminated" << std::endl;
  std::cout << "[supervisor] : Control App terminated" << std::endl;
  std::cout << "[supervisor] : Software Bus App terminated" << std::endl;
  return 0;
}
