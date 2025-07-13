#include <iostream>

// i2c를 위한.. linux kernel 이용하는 거임
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>  // ioctl()
#include <fcntl.h>      // open(), O_RDWR
#include <unistd.h>     // read(), write(), close(), usleep()


#include "sensor.h"


int main(){
  
  BMP390 baro;
  baro.init();
  usleep(2000000);
  for(int i=0; i<2; i++){
    baro.read_data();
    usleep(100000);
  }
  return 0;
}
