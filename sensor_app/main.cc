#include <iostream>
#include <vector>

// i2c를 위한.. linux kernel 이용하는 거임
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>  // ioctl()
#include <fcntl.h>      // open(), O_RDWR
#include <unistd.h>     // read(), write(), close(), usleep()

// threading 
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>


// my includes
#include "sensor.h"


std::mutex i2c_mutex; // i2c bus mutex
std::atomic<bool> keep_running(true);

void thread_read_sensor(Sensor& sensor, int period){
  std::vector<float> data;
  // period should be 'ms' unit
  while(keep_running){
    {
    std::lock_guard<std::mutex> lock(i2c_mutex);
    sensor.read_data();
    } // mutex lock scope.. scope 나가면 자동 unlock
    data = sensor.get_data();
    for(int i=0; i<data.size(); i++){
      std::cout << data[i] << " ";
    }
    std::cout << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(period));
  }
}

int main(){
  
  BMP390 baro;
  BNO055 imu;

  std::vector<float> baro_data;
  std::vector<float> imu_data;
  

  baro.init();
  usleep(1000000); // 1 sec
  imu.init();
  usleep(1000000); // 1 sec
  
  /*
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
  */
  
  std::thread barometer_thread(thread_read_sensor, std::ref(baro), 100);
  std::thread imu_thread(thread_read_sensor, std::ref(imu), 100);
  
  std::this_thread::sleep_for(std::chrono::seconds(100)); // 5초 후 종료
  keep_running = false;
  
  barometer_thread.join();
  imu_thread.join();

  return 0;
}
