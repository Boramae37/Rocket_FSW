#include <iostream>
#include "shape.h"

int main() {
  int size;
  Rectangle nemo(3, 10);


  size = nemo.GetSize();

  std::cout << "Get Size : " << size << std::endl;

  std::system("Pause");
  
  return 0;
}