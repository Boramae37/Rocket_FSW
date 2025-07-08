#include <iostream>
#include <unistd.h>
#include <sys/types.h> // pid_t
#include <sys/wait.h> // waitpid()
#include <cstdlib>  // exit()


#define sensor_app 0
#define com_app 1
#define ctrl_app 2

int main() {
  
  pid_t pid[3];

  pid[sensor_app] = fork(); // fork()를 만드는 순간, 부모와 자식 프로세스가 복제되고, 각각의 pid[sensor_app]에 숫자가 반환된다. 부모에는 자식의 pid, 자식에게는 0
  


  // 0이면 자식이란 말이기에..
  if(pid[sensor_app] == 0){

    std::cout << "PID["<< getpid() <<"] Launch Sensor Application" << std::endl;
    // 새로운 프로그램으로 덮어 씌운다.
    execl("../../sensor_app/run/sensor", "sensor", NULL);
    std::cout << "Failed to launch Sensor Application" << std::endl;
    exit(1); 
  }
  else {
    int sensor_status;
    waitpid(pid[sensor_app], &sensor_status, 0);
    std::cout << "[supervisor] : Sensor App terminated" << std::endl;
  }

  return 0;
}
