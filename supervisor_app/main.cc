#include <iostream>
#include <string>
#include "communication.h"
#include <pigpio.h>
#include <thread>
#include <unistd.h>
#include <sys/types.h> // pid_t

int main() {
  
  std::string str;

  packet a;

  str = a.packet_combine();

  std::cout << str << std::endl;
  
  pid_t pid[3];

  pid[0] = fork();
  pid[1] = fork();

  std::cout << pid[0] << "and" << pid[1] << std::endl;

  return 0;
}
