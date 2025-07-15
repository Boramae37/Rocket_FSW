#include <iostream>

// i2c를 위한.. linux kernel 이용하는 거임
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>  // ioctl()
#include <fcntl.h>      // open(), O_RDWR
#include <unistd.h>     // read(), write(), close(), usleep()

// my includes
#include "sensor.h"


int main(){
  
  BMP390 baro;
  BNO055 imu;

  std::vector<float> baro_data;
  std::vector<float> imu_data;
  

  baro.init();
  usleep(10000);
  imu.init();
  usleep(2000000);
  for(int i=0; i<1000; i++){
    baro.read_data();
    baro_data = baro.get_data();
    std::cout << "Barometer Data: Pressure = " << baro_data[0] 
              << ", Temperature = " << baro_data[1] << std::endl;
    usleep(20000);
    imu.read_data();
    usleep(20000);
    imu_data = imu.get_data();
    std::cout << "IMU Data: Acceleration = (" << imu_data[0] << ", " << imu_data[1] << ", " << imu_data[2] << "), "
              << "Gyroscope = (" << imu_data[3] << ", " << imu_data[4] << ", " << imu_data[5] << "), "
              << "Euler Angles = (" << imu_data[6] << ", " << imu_data[7] << ", " << imu_data[8] << "), "
              << "Quaternion = (" << imu_data[9] << ", " << imu_data[10] << ", " << imu_data[11] << ", " << imu_data[12] << "), "
              << "Linear Acceleration = (" << imu_data[13] << ", " << imu_data[14] << ", " << imu_data[15] << ")" 
              << std::endl;
    usleep(20000);
  }


  return 0;
}
