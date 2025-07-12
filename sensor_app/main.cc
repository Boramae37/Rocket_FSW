#include <iostream>

// i2c를 위한.. linux kernel 이용하는 거임
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>  // ioctl()
#include <fcntl.h>      // open(), O_RDWR
#include <unistd.h>     // read(), write(), close()


#include "sensor.h"


int main(){
  
  BMP390 baro;
  baro.init();
  baro.read_data();

  return 0;
}
