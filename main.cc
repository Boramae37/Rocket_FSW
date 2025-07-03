#include <iostream>
#include <string>
#include "communication.h"

int main() {
  
  std::string str;

  packet a;

  str = a.packet_combine();

  std::cout << str << std::endl;

}