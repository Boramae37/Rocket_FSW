#include "sensor.h"
#include <iostream>
#include <cerrno>
#include <cstring>

#include <stdio.h>

#include <iomanip>


// i2c를 위한.. linux kernel 이용하는 거임
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>  // ioctl()
#include <unistd.h> // write, read, close
#include <fcntl.h>  // open
                    //
                    //
         
/* Boash provide great data sheet
 * but, they provide bull shit API. (of course, because of my limitation)
 * It's fucking unkind. They don't say What I have to modify
 * If you want to use bmp3 API, you have to read all of .c and .h files,
 * find what function you have to make or modify, and make them following 
 * the condition.
 * Of course it's possible. but I recommend you just read the datasheet,
 * find register map, and make functions. It's better to maintain codes.
*/

#define BMP390_I2C_ADDR 0x77 // 0x76 or 0x77

#define BMP390_PRESSURE_ADDR_0 0x09 // MSB
#define BMP390_PRESSURE_ADDR_1 0x08
#define BMP390_PRESSURE_ADDR_2 0x07 // LSB


#define BMP390_CMD_ADDR 0x7E;







/*
// file descriptor
// in BMP390, a register has 8bits,
// so you can use unit8_t or unsigned char for addr
struct I2C_FD{
  int fd;
  unsigned long addr; // i2c slave address
};
*/

int i2c_write(uint8_t slave_addr, uint8_t write_reg, uint8_t write_data){
  
  int fd;
  
  char path[] = "/dev/i2c-1"; // linux i2c device path
  
  uint8_t write_arr[2]; // 쓰는 경우 보통 대상, 값 쌍으로 2개만.. 그리고 보통 1개의 register만 변경함.  

  fd = open(path, O_RDWR);
  
  if (fd < -1){
    perror("[ERROR] : Failed to open I2C");
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to open I2C" << slave_addr << std::endl; 
    return 0;
  }
  
  if(ioctl(fd, I2C_SLAVE, slave_addr) < 0){
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to open I2C" << slave_addr << std::endl; 
    return 0;
  }

  if (write(fd, write_arr ,2) != 2){
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to I2C write" << slave_addr << std::endl;
    return 0;
  }
  
  std::cout << "Writing Complete" << std::endl;
  return 1;

}






/* bmp390 i2c read function
* data array : i think using reference is better.. 
* len means length of both read_addr and data_arr.
* return value 0 = error, 1 = success*/

/* how to read multiple register? bmp390 has auto-increment.
 * so, you just tell it the start register address by open() function,
 * and read the desired number of bits by read() function
 * */
unsigned int i2c_read(uint8_t slave_addr, uint8_t read_addr, int len){
  
  int fd;
  
  char path[] = "/dev/i2c-1"; // linux i2c device path
  
  //std::vecotr<uint8_t> data_arr[len];
  uint8_t data_arr[3]; // 어차피 데이터 값이 3바이트, 나머지는 다 그 이하임..
  

  fd = open(path, O_RDWR);
  
  if (fd < -1){
    perror("[ERROR] : Failed to open I2C");
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to open I2C" << path << std::endl; 
    return 0;
  }
  
  if(ioctl(fd, I2C_SLAVE, slave_addr) < 0){
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to open I2C" << path << std::endl; 
    return 0;
  }
  
  // write에 fd, pointer, byte 수 를 전달해줘야함. pointer는 내가 보낼 데이터의 포인터 변수임.
  if(write(fd, &read_addr, 1) != 1){ // wirte 성공시 써준 개수만큼..
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to write slave" << path << std::endl;
    return 0;
  }
  
  
  if (read(fd, data_arr, len) < 0){
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to read I2C" << path << std::endl;  
    return 0;  
  }
  close(fd); 
  
  unsigned int result = 0;
  for (int i = 0; i < len; i++){

    // Little-endian
    result |= static_cast<unsigned int>(data_arr[i]) << (8*i);

  }
  
  std::cout << "combined 3 bytes : "
            << std::uppercase         // 대문자 쓰기
            << std::setfill('0')      // 빈자리 0채우기
            << std::setw(6)           // 6자리 고정
            << std::hex               // 16진수
            << result
            << "(" << std::dec << result << ")" << std::endl; // 10진수 출력
  
  
  
  return result; // sucess
}












// Most Vexing Parse error.. 생성자는 기본 쓰거나 무조건 인자 받도록 하자.. default 인자 ㄴㄴ. init에서 처리하자.
// BMP390::BMP390(){}

// BMP3_ADDR_I2C_PRIM    UINT8_C(0x76)
// BMP3_ADDR_I2C_SEC     UINT8_C(0x77)
bool BMP390::init(){
  
  this->slave_addr = BMP390_I2C_ADDR;
  for (int i=0; i<3; i++){
    this->row_data[i] = 0;
  }

  // Trimming Coefficients (only for pressure)
  i2c_read(this->slave_addr, 0x36, 2)

  // reset 기능 넣기
  // mode normal 설정
  // osr 등 설정
  // calibration data 가져오기
  std::cout << "BMP390 initialized" << std::endl;
  return true;
}





void BMP390::read_data(){
  // read pressure
  this->pressure = i2c_read(this->slave_addr, BMP390_PRESSURE_ADDR_2, 3); 

  i2c_read(this->slave_addr, 0x00, 1);
  /* parse, compensate 추가
   * this->pressure_data, barometer_data에 대입*/
  

}


void BMP390::reset(){
  // 0XB6 : softreset command
  i2c_write(this->slave_addr, BMP390_CMD_ADDR, 0xB6);
  
}


//virtual BMP390::~BMP390(){}


