#include "sensor.h"
#include <iostream>
#include <cerrno>
#include <cstring>
#include <vector>
#include <stdio.h>

#include <iomanip>
#include <string_view> // 문자열 뷰를 위해
#include <charconv> // std::to_chars

#include <cmath> // pow() for barometric formula

#include "spdlog/fmt/fmt.h" // fmt library for formatting strings

// i2c를 위한.. linux kernel 이용하는 거임
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>  // ioctl()
#include <unistd.h> // write, read, close, usleep()
#include <fcntl.h>  // open
                    //
                    //


// pigpio 사용위함(gps 센서용.. soft uart 사용해야함... xbee가 hardware uart를 사용하기에)
#include <pigpiod_if2.h>

// Raspberry Pi I2C device path
constexpr std::string_view LINUX_I2C_PATH = "/dev/i2c-1"; 


/* Boash provide great data sheet
 * but, they provide bull shit API. (of course, because of my limitation)
 * It's fucking unkind. They don't say What I have to modify
 * If you want to use bmp3 API, you have to read all of .c and .h files,
 * find what function you have to make or modify, and make them following 
 * the condition.
 * Of course it's possible. but I recommend you just read the datasheet,
 * find register map, and make functions. It's better to maintain codes.
*/

////////////// BMP390 //////////////////////////////////////////////////////////////////////

#define BMP390_I2C_ADDR 0x77 // 0x76 or 0x77

#define BMP390_PRESSURE_ADDR_0 0x09 // MSB
#define BMP390_PRESSURE_ADDR_1 0x08
#define BMP390_PRESSURE_ADDR_2 0x07 // LSB

#define BMP390_PWRCTRL_ADDR 0x1B // power mode select. sleep(00), normal(11), forced(10,01) + p,t enable check sheet
#define BMP390_PWR_MODE 0x33 // normal, both pressure and temperature enabled

#define BMP390_CMD_ADDR 0x7E

// constants fot compensating data (bmp390)
#define const_2p3 8.0f
#define const_2p6 64.0f
#define const_2p8 256.0f
#define const_2p14 16384.0f
#define const_2p15 32768.0f
#define const_2p20 1048576.0f
#define const_2p29 536870912.0
#define const_2p30 1073741824.0
#define const_2p32 4294967296.0f
#define const_2p37 137438953472.0f
#define const_2p48 281474976710656.0f
#define const_2p65 36893488147419103232.0f

constexpr double SEA_LEVEL_PRESSURE = 101325.0; // sea level pressure in pascal


////////// BNO055 Sensor //////////////////////////////////////////////////////
#define BNO055_I2C_ADDR 0x28 // 0x28, 0x29


// 선언부 (추후 헤더로 옮김)
int i2c_write(uint8_t slave_addr, uint8_t write_reg, uint8_t write_data);
uint32_t i2c_read(uint8_t slave_addr, uint8_t read_addr, int len);



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
  
  const char* path = LINUX_I2C_PATH.data(); // linux i2c device path
  
  uint8_t write_arr[2]; // 쓰는 경우 보통 대상, 값 쌍으로 2개만.. 그리고 보통 1개의 register만 변경함.  

  write_arr[0] = write_reg;
  write_arr[1] = write_data;

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
  close(fd);

  fmt::print("Writing Complete\n");
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

 // read data, and convert data to uint32_t, and return it. 
bool read_status = false;
uint32_t i2c_read(uint8_t slave_addr, uint8_t read_addr, int len){
  
  if (len < 1 || len > 4){
    std::cerr << "[ERROR] : Invalid length for I2C read. Length must be between 1 and 3." << std::endl;
    std::cerr << "[ERROR] : If you want more than 4, use i2c_read_64" << len << std::endl;
    return 0; // error
  }

  int fd;
  
  const char* path = LINUX_I2C_PATH.data(); // linux i2c device path
  
  //std::vecotr<uint8_t> data_arr[len];
  uint8_t data_arr[4]; // 어차피 데이터 값이 3바이트, 나머지는 다 그 이하임.. 근데 혹시 모르니 4바이트까지 커버..통일 위해
  

  fd = open(path, O_RDWR);
  // open check
  if (fd < -1){
    perror("[ERROR] : Failed to open I2C");
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to open I2C" << path << std::endl; 
    read_status = false;
    return 0;
  }
  // i2c bus set and check
  if(ioctl(fd, I2C_SLAVE, slave_addr) < 0){
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to open I2C" << path << std::endl; 
    read_status = false;
    return 0;
  }
  
  // write에 fd, pointer, byte 수 를 전달해줘야함. pointer는 내가 보낼 데이터의 포인터 변수임.
  if(write(fd, &read_addr, 1) != 1){ // wirte 성공시 써준 개수만큼..
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to write slave for read" << path << std::endl;
    read_status = false;
    return 0;
  }
  
  // read data ( fd, 읽은 데이터 저장할 곳, 길이) => 읽을 데이터가 있는 레지스터 주소는 write으로 가리킴. 일종의 file pointer 설정으로 write으로 한다보면됨.
  if (read(fd, data_arr, len) < 0){
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to read I2C" << path << std::endl;  
    read_status = false;
    return 0;  
  }
  close(fd); 
  
  // make return data
  uint32_t result = 0;
  for (int i = 0; i < len; i++){
    // Little-endian
    result |= static_cast<uint32_t>(data_arr[i]) << (8*i);
  }
  
  /*
  std::cout << "Reading" << len << "bytes : "
            << std::uppercase         // 대문자 쓰기
            << std::setfill('0')      // 빈자리 0채우기
            << std::setw(6)           // 6자리 고정
            << std::hex               // 16진수
            << result
            << "(" << std::dec << result << ")" << std::endl; // 10진수 출력
  */
  read_status = true;
  return result; // sucess
}

uint64_t i2c_read_64(uint8_t slave_addr, uint8_t read_addr, int len){
  
  if (len < 1 || len > 8){
    std::cerr << "[ERROR] : Invalid length for I2C read. Length must be between 1 and 8." << std::endl;
    return 0; // error
  }

  int fd;
  
  const char* path = LINUX_I2C_PATH.data(); // linux i2c device path
  
  //std::vecotr<uint8_t> data_arr[len];
  uint8_t data_arr[8]; // 64bits = 8bytes  

  fd = open(path, O_RDWR);
  // open check
  if (fd < -1){
    perror("[ERROR] : Failed to open I2C");
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to open I2C" << path << std::endl; 
    read_status = false;
    return 0;
  }
  // i2c bus set and check
  if(ioctl(fd, I2C_SLAVE, slave_addr) < 0){
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to open I2C" << path << std::endl; 
    read_status = false;
    return 0;
  }
  
  // write에 fd, pointer, byte 수 를 전달해줘야함. pointer는 내가 보낼 데이터의 포인터 변수임.
  if(write(fd, &read_addr, 1) != 1){ // wirte 성공시 써준 개수만큼..
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to write slave for read" << path << std::endl;
    read_status = false;
    return 0;
  }
  
  // read data ( fd, 읽은 데이터 저장할 곳, 길이) => 읽을 데이터가 있는 레지스터 주소는 write으로 가리킴. 일종의 file pointer 설정으로 write으로 한다보면됨.
  if (read(fd, data_arr, len) < 0){
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to read I2C" << path << std::endl;  
    read_status = false;
    return 0;  
  }
  close(fd); 
  
  // make return data
  uint64_t result = 0;
  for (int i = 0; i < len; i++){
    // Little-endian
    result |= static_cast<uint64_t>(data_arr[i]) << (8*i);
  }
  /*
  std::cout << "Reading" << len << "bytes : "
            << std::uppercase         // 대문자 쓰기
            << std::setfill('0')      // 빈자리 0채우기
            << std::setw(6)           // 6자리 고정
            << std::hex               // 16진수
            << result
            << "(" << std::dec << result << ")" << std::endl; // 10진수 출력
  */
  read_status = true;
  return result; // sucess
}



// write을 통한 register 지정 없이 read만 하는 함수.. bmp390 normal mode 처럼 알아서 register 위치 맞춰주면 read만 하면 됨.
uint32_t i2c_read_only(uint8_t slave_addr, uint8_t read_addr, int len){

  int fd;
  
  const char* path = LINUX_I2C_PATH.data(); // linux i2c device path
  
  //std::vecotr<uint8_t> data_arr[len];
  uint8_t data_arr[4]; // 어차피 데이터 값이 3바이트, 나머지는 다 그 이하임.. 하지만 함수명 상 4바이트까지 가능해야 이치에 맞다.
  

  fd = open(path, O_RDWR);
  // open check
  if (fd < -1){
    perror("[ERROR] : Failed to open I2C");
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to open I2C" << path << std::endl; 
    read_status = false;
    return 0;
  }
  // i2c bus set and check
  if(ioctl(fd, I2C_SLAVE, slave_addr) < 0){
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to open I2C" << path << std::endl; 
    read_status = false;
    return 0;
  }
  
   
  // read data ( fd, 읽은 데이터 저장할 곳, 길이) => 읽을 데이터가 있는 레지스터 주소는 write으로 가리킴. 일종의 file pointer 설정으로 write으로 한다보면됨.
  if (read(fd, data_arr, len) < 0){
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to read I2C" << path << std::endl;  
    read_status = false;
    return 0;  
  }
  close(fd); 
  
  // make return data
  uint32_t result = 0;
  for (int i = 0; i < len; i++){
    // Little-endian
    result |= static_cast<uint32_t>(data_arr[i]) << (8*i);
  }
  
  // std::cout << "Reading" << len << "bytes : "
  //           << std::uppercase         // 대문자 쓰기
  //           << std::setfill('0')      // 빈자리 0채우기
  //           << std::setw(6)           // 6자리 고정
  //           << std::hex               // 16진수
  //           << result
  //           << "(" << std::dec << result << ")" << std::endl; // 10진수 출력
  
  read_status = true;
  return result; // sucess

}

uint64_t i2c_read_only_64(uint8_t slave_addr, uint8_t read_addr, int len){

  int fd;
  
  const char* path = LINUX_I2C_PATH.data(); // linux i2c device path
  
  // data 크기 맞는..
  uint8_t data_arr[8]; //   

  fd = open(path, O_RDWR);
  // open check
  if (fd < -1){
    perror("[ERROR] : Failed to open I2C");
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to open I2C" << path << std::endl; 
    read_status = false;
    return 0;
  }
  // i2c bus set and check
  if(ioctl(fd, I2C_SLAVE, slave_addr) < 0){
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to open I2C" << path << std::endl; 
    read_status = false;
    return 0;
  }
  
   
  // read data ( fd, 읽은 데이터 저장할 곳, 길이) => 읽을 데이터가 있는 레지스터 주소는 write으로 가리킴. 일종의 file pointer 설정으로 write으로 한다보면됨.
  if (read(fd, data_arr, len) < 0){
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to read I2C" << path << std::endl;  
    read_status = false;
    return 0;  
  }
  close(fd); 
  
  // make return data
  uint64_t result = 0;
  for (int i = 0; i < len; i++){
    // Little-endian
    result |= static_cast<uint64_t>(data_arr[i]) << (8*i);
  }
  
  // std::cout << "Reading" << len << "bytes : "
  //           << std::uppercase         // 대문자 쓰기
  //           << std::setfill('0')      // 빈자리 0채우기
  //           << std::setw(6)           // 6자리 고정
  //           << std::hex               // 16진수
  //           << result
  //           << "(" << std::dec << result << ")" << std::endl; // 10진수 출력
  
  read_status = true;
  return result; // sucess

}



/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////


BMP 390 member functions


///////////////////////////////////////////////////////////////////////////////////////////////////////////// */



// Most Vexing Parse error.. 생성자는 기본 쓰거나 무조건 인자 받도록 하자.. default 인자 ㄴㄴ. init에서 처리하자.
// BMP3_ADDR_I2C_PRIM    UINT8_C(0x76)
// BMP3_ADDR_I2C_SEC     UINT8_C(0x77)
BMP390::BMP390() : slave_addr{BMP390_I2C_ADDR} {}


bool BMP390::init(){
  
  // I2c slave address set 
  //this->slave_addr = BMP390_I2C_ADDR;
  
  // soft reset
  fmt::print("Sensor soft resetting....\n");
  this->reset();
  usleep(2000000);
  fmt::print("Sensor Soft reset complete\n");

  ///////////////////// SENSOR MODE SETTING ///////////////////////////////////////////////////////////////////
  // pwr mode setting (00110011 = normal mode, both pressure, temperature enabled)
  i2c_write(this->slave_addr, BMP390_PWRCTRL_ADDR, BMP390_PWR_MODE);
  fmt::print("Power mode setting complete\n");
  usleep(2000000);


  // osr (oversampling rates) 
  // osr_t = x1 / osr_p = x4 (010, standard resolution) => Register: 0x1C , value: 0x02 
  
  i2c_write(this->slave_addr, 0x1C, 0x02);

  // ocr (Output Data Rate) max : 200Hz
  // select : 25Hz => Register : 0x1D, value : 0x03
  i2c_write(this->slave_addr, 0x1D, 0x03);

  // iir filter : on, Register : 0x1F, 321 bit : 010 (filter coefficient = 3) => 0x04
  i2c_write(this->slave_addr, 0x1F, 0x04);


  // Trimming Coefficients : Do not chage
  // temperature
  this->nvm_calib_data.par_t1 = static_cast<uint16_t>(i2c_read(this->slave_addr, 0x31, 2));
  this->nvm_calib_data.par_t2 = static_cast<uint16_t>(i2c_read(this->slave_addr, 0x33, 2));
  this->nvm_calib_data.par_t3 = static_cast<int8_t>(i2c_read(this->slave_addr, 0x35, 1));
  // pressure
  this->nvm_calib_data.par_p1 = static_cast<int16_t>(i2c_read(this->slave_addr, 0x36, 2));
  this->nvm_calib_data.par_p2 = static_cast<int16_t>(i2c_read(this->slave_addr, 0x38, 2));
  this->nvm_calib_data.par_p3 = static_cast<int8_t>(i2c_read(this->slave_addr, 0x3A, 1));
  this->nvm_calib_data.par_p4 = static_cast<int8_t>(i2c_read(this->slave_addr, 0x3B, 1));
  this->nvm_calib_data.par_p5 = static_cast<uint16_t>(i2c_read(this->slave_addr, 0x3C, 2));
  this->nvm_calib_data.par_p6 = static_cast<uint16_t>(i2c_read(this->slave_addr, 0x3E, 2));
  this->nvm_calib_data.par_p7 = static_cast<int8_t>(i2c_read(this->slave_addr, 0x40, 1));
  this->nvm_calib_data.par_p8 = static_cast<int8_t>(i2c_read(this->slave_addr, 0x41, 1));
  this->nvm_calib_data.par_p9 = static_cast<int16_t>(i2c_read(this->slave_addr, 0x42, 2));
  this->nvm_calib_data.par_p10 = static_cast<int8_t>(i2c_read(this->slave_addr, 0x44, 1));
  this->nvm_calib_data.par_p11 = static_cast<int8_t>(i2c_read(this->slave_addr, 0x45, 1));
  
  // convert calib_data into floating point number.. (parse data)
  this->calib_data_t[0] = static_cast<float>(this->nvm_calib_data.par_t1)*const_2p8;
  this->calib_data_t[1] = static_cast<float>(this->nvm_calib_data.par_t2)/const_2p30;
  this->calib_data_t[2] = static_cast<float>(this->nvm_calib_data.par_t3)/const_2p48;
  
  this->calib_data_p[0] = (static_cast<float>(this->nvm_calib_data.par_p1)- const_2p14) / const_2p20;
  this->calib_data_p[1] = (static_cast<float>(this->nvm_calib_data.par_p2)- const_2p14) / const_2p29;
  this->calib_data_p[2] = static_cast<float>(this->nvm_calib_data.par_p3) / const_2p32;
  this->calib_data_p[3] = static_cast<float>(this->nvm_calib_data.par_p4) / const_2p37;
  this->calib_data_p[4] = static_cast<float>(this->nvm_calib_data.par_p5) * const_2p3;
  this->calib_data_p[5] = static_cast<float>(this->nvm_calib_data.par_p6) / const_2p6;
  this->calib_data_p[6] = static_cast<float>(this->nvm_calib_data.par_p7) / const_2p8;
  this->calib_data_p[7] = static_cast<float>(this->nvm_calib_data.par_p8) / const_2p15;
  this->calib_data_p[8] = static_cast<float>(this->nvm_calib_data.par_p9) / const_2p48;
  this->calib_data_p[9] = static_cast<float>(this->nvm_calib_data.par_p10) / const_2p48;
  this->calib_data_p[10] = static_cast<float>(this->nvm_calib_data.par_p11) / const_2p65;
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // 여기서 한 번 읽으면, 이후 read only 함수로 read_data 구현 가능..  

  fmt::print("BMP390 initialized\n");
  return true;
}




void BMP390::read_data(){
  // read pressure
  this->pressure = i2c_read(this->slave_addr, BMP390_PRESSURE_ADDR_2, 3); //  
  this->temperature = i2c_read(this->slave_addr, 0x07, 3); // 섭씨 단위
  //i2c_read(this->slave_addr, 0x03, 1);
  
  /* parse, compensate 추가
   * this->pressure, temperature에 대입*/
  if (read_status){
    this->compensate_data();
    this->pressure = 44330.0 * (1.0 - pow((this->pressure / SEA_LEVEL_PRESSURE), 0.1903)); // barometric formula
  }
  else{
    fmt::print("Failed to read\n");
  }
}






void BMP390::compensate_data(){
  // temperature
  float t_partial_data1;
  float t_partial_data2;

  t_partial_data1 = static_cast<float>(this->temperature - this->calib_data_t[0]);
  t_partial_data2 = static_cast<float>(t_partial_data1 * this->calib_data_t[1]);

  this->temperature = t_partial_data2 + (t_partial_data1 * t_partial_data1)*this->calib_data_t[2];
  

  // pressurre
  float partial_data1;
  float partial_data2;
  float partial_data3;
  float partial_data4;
  float partial_out1;
  float partial_out2;

  partial_data1 = this->calib_data_p[5] * this->temperature;
  partial_data2 = this->calib_data_p[6] * (this->temperature * this->temperature);
  partial_data3 = this->calib_data_p[7] * (this->temperature * this->temperature * this->temperature);
  partial_out1 = this->calib_data_p[4] + partial_data1 + partial_data2 + partial_data3;

  partial_data1 = this->calib_data_p[1] * this->temperature;
  partial_data2 = this->calib_data_p[2] * (this->temperature * this->temperature);
  partial_data3 = this->calib_data_p[3] * (this->temperature * this->temperature * this->temperature);
  partial_out2 = static_cast<float>(this->pressure * (this->calib_data_p[0] + partial_data1 + partial_data2 + partial_data3));

  partial_data1 = this->pressure * this->pressure;
  partial_data2 = this->calib_data_p[8] + this->calib_data_p[9] * this->temperature;
  partial_data3 = partial_data1 * partial_data2;
  partial_data4 = partial_data3 + ((this->pressure*this->pressure*this->pressure)*this->calib_data_p[10]);
  this->pressure = partial_out1 + partial_out2 + partial_data4;
}


void BMP390::reset(){
  // 0XB6 : softreset command
  i2c_write(this->slave_addr, BMP390_CMD_ADDR, 0xB6);
  
}

std::vector<float> BMP390::get_data(){
  return {this->pressure, this->temperature};
}


BMP390::~BMP390(){}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////






////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
///
///
///
///       BNO055 Sensor member function
///
///
///
///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



BNO055::BNO055() : slave_addr{BNO055_I2C_ADDR} {}
BNO055::~BNO055(){}

bool BNO055::init(){
  // Initialize the BNO055 sensor
  // Set the I2C slave address (0x28)
  //this->slave_addr = BNO055_I2C_ADDR; // Default I2C address for B
  
  // Soft reset the sensor
  fmt::print("BNO055 soft resetting...\n");
  this->reset();
  fmt::print("BNO055 soft reset complete\n");


  // power mode : default mode is NORMAL so, we don't need to set it.
  // if you want to change, set resgister 0x3E


  // Operating mode : default is Fusion mode(in reset function, the mode is chagned to config mode and 
  //                  fusion-imu mode again), so we don't need to set it.
  // if you want to change, set register 0x3D
  // i2c_write(this->slave_addr, 0x3D, 0x18);


  // Unit setting : default is m/s^2 for acceleration, degrees/s for gyroscope
  // if you want to change, set register 0x3B


  // Each sensor(accelerometer, gyroscope, magnetometer) configuration
  // we will use Fusion mode, so we don't need to set it.
  // Becaues, fusion mode automatically control the configuration of each sensor.


  // Read the system status register 0x39
  /* this->status = i2c_read(this->slave_addr, 0x39, 1);
  if (status == 0x01) {
    this->status = i2c_read(this->slave_addr, 0x3A, 1);
    std::cerr << "[ERROR] : BNO055 not responding. Status: " << std::hex << static_cast<int>(status) << std::endl;
    return false;
  }
  */

  fmt::print("BNO055 initialized\n");
  return true;
}


void BNO055::reset(){
  // Soft reset command for BNO055
  // soft reset command is not exist. So, by mode change, we can reset the sensor.
  // because, when it changed to config mode, it reset all registers to default values.
  i2c_write(this->slave_addr, 0x3D, 0x10);
  
  // after changing to config mode, we have to select operating mode again
  i2c_write(this->slave_addr, 0x3D, 0x18);
  usleep(200000); // Wait for the reset to complete
  
}


void BNO055::read_data(){
  // we will use Fusion mode.
  // Read accelerometer and gyroscope data from BNO055
  // unit is m/s^2, degrees/s
  uint64_t accel_data = i2c_read_64(this->slave_addr, 0x08, 6); // Read 6 bytes for accelerometer data
  uint64_t gyro_data = i2c_read_64(this->slave_addr, 0x14, 6); // Read 6 bytes for gyroscope data

  uint64_t euler_data = i2c_read_64(this->slave_addr, 0x1A, 6); // Read 6 bytes for Euler angles
  uint64_t quaternion_data = i2c_read_64(this->slave_addr, 0x20, 8); // Read 8 bytes for Quaternion data

  uint64_t linear_acc_data = i2c_read_64(this->slave_addr, 0x28, 6); // Read 6 bytes for linear acceleration data

  // Parse the data
  // fusion mode automatically parse the data.
  // but, we need to compensate the data one more time.
  // so, compensating function will be added later.


  // Store the parsed data in the class members
  // 16bit unsigned integer should be converted to int16_t first, then to float
  // because, BNO055 uses 16bit signed integer for acceleration, gyroscope,and etc.
  this->acc_x = static_cast<float>(static_cast<int16_t>(accel_data & 0xFFFF));
  this->acc_y = static_cast<float>(static_cast<int16_t>((accel_data >> 16) & 0xFFFF));
  this->acc_z = static_cast<float>(static_cast<int16_t>((accel_data >> 32) & 0xFFFF));
  this->gyro_x = static_cast<float>(static_cast<int16_t>(gyro_data & 0xFFFF));
  this->gyro_y = static_cast<float>(static_cast<int16_t>((gyro_data >> 16) & 0xFFFF));
  this->gyro_z = static_cast<float>(static_cast<int16_t>((gyro_data >> 32) & 0xFFFF));
  this->Euler_ang_x = static_cast<float>(static_cast<int16_t>(euler_data & 0xFFFF));
  this->Euler_ang_y = static_cast<float>(static_cast<int16_t>((euler_data >> 16) & 0xFFFF));
  this->Euler_ang_z = static_cast<float>(static_cast<int16_t>((euler_data >> 32) & 0xFFFF));
  this->Quternion_ang_w = static_cast<float>(static_cast<int16_t>(quaternion_data & 0xFFFF));
  this->Quternion_ang_x = static_cast<float>(static_cast<int16_t>((quaternion_data >> 16) & 0xFFFF));
  this->Quternion_ang_y = static_cast<float>(static_cast<int16_t>((quaternion_data >> 32) & 0xFFFF));
  this->Quternion_ang_z = static_cast<float>(static_cast<int16_t>((quaternion_data >> 48) & 0xFFFF));
  this->linear_acc_x = static_cast<float>(static_cast<int16_t>(linear_acc_data & 0xFFFF));
  this->linear_acc_y = static_cast<float>(static_cast<int16_t>((linear_acc_data >> 16) & 0xFFFF));
  this->linear_acc_z = static_cast<float>(static_cast<int16_t>((linear_acc_data >> 32) & 0xFFFF));

  // LSB -> m/s^2, degrees/s 등등.. 필요없는 듯
}

// return the sensor data as a vector
std::vector<float> BNO055::get_data(){
  // Return the sensor data as a vector
  return {this->acc_x, this->acc_y, this->acc_z, 
          this->gyro_x, this->gyro_y, this->gyro_z,
          this->Euler_ang_x, this->Euler_ang_y, this->Euler_ang_z,
          this->Quternion_ang_w, this->Quternion_ang_x, this->Quternion_ang_y, this->Quternion_ang_z,
          this->linear_acc_x, this->linear_acc_y, this->linear_acc_z};
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
///
///
///
///       ULTIMATE GPS MODULE member function
///
///
///
///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ULTIMATEGPS::ULTIMATEGPS(int baud_rate, int tx, int rx) : BAUD_RATE(baud_rate), TX_PIN(tx), RX_PIN(rx) {
  int ret;
  this->pi = pigpio_start(NULL, NULL); // Initialize pigpio library
  if (this->pi < 0) {
    std::cerr << "[ERROR] : Failed to initialize pigpio library" << std::endl;
    while(this->pi < 0){
      usleep(100000); // Wait for 100ms before retrying
      fmt::print("[SENSOR][GPS][ERROR] : Retrying to initialize pigpio library...\n");
      this->pi = pigpio_start(NULL, NULL); // Initialize pigpio library
    }
    fmt::print("[SENSOR][GPS] : pigpio library initialized successfully\n");
  }
  fmt::print("[SENSOR][GPS] : pigpio started successfully\n");


  ret = bb_serial_read_open(this->pi, this->RX_PIN, this->BAUD_RATE, 8); // Open the serial port for reading
  while(ret < 0){
    std::cerr << "[ERROR] : Failed to open serial port for reading" << std::endl;
    usleep(100000); // Wait for 100ms before retrying
    fmt::print("[SENSOR][GPS][ERROR] : Retrying to open serial port for reading...\n");
    ret = bb_serial_read_open(this->pi, this->RX_PIN, this->BAUD_RATE, 0); // Open the serial port for reading
  }

  fmt::print("[SENSOR][GPS] : Serial port opened for reading successfully\n");
  memset(this->buffer, 0, sizeof(this->buffer)); // Initialize the buffer to zero
}


ULTIMATEGPS::~ULTIMATEGPS() {
  bb_serial_read_close(this->pi, this->RX_PIN);
  pigpio_stop(this->pi); // Stop pigpio library
}

bool ULTIMATEGPS::init(){return true;}


void ULTIMATEGPS::read_data(){
  memset(this->buffer, '0', sizeof(this->buffer));
  bb_serial_read(this->pi, this->RX_PIN, this->buffer, sizeof(this->buffer) - 1); // Read data from the serial port
  this->buffer[sizeof(this->buffer) - 1] = '\0'; // Null-terminate the string
  fmt::print("[SENSOR][GPS] : Read data from GPS: {}\n", this->buffer); // Print the read data

  std::string_view view(this->buffer);

  this->start_idx = view.find("$GPGGA");
  this->end_idx = view.find("\r\n", this->start_idx);

  std::string_view gpgga_message = view.substr(this->start_idx, this->end_idx - this->start_idx);
  
  std::vector<std::string_view> fields;
  size_t curret_pos = 0;

  while(true){
    size_t next_comma = gpgga_message.find(',', curret_pos);

    fields.push_back(gpgga_message.substr(curret_pos, next_comma - curret_pos));
    if (next_comma == std::string_view::npos) {
      break; // No more commas found, exit the loop
    }
    // Move to the next position after the comma
    curret_pos = next_comma + 1;
  }

  if (fields.size() > 5){
    if(fields[6] != "0"){
      // std::string_view lat_str = fields[2];
      // std::string_view ns_indicator = fields[3];
      // std::string_view lon_str = fields[4];
      // std::string_view ew_indicator = fields[5];

      std::from_chars(fields[2].data(), fields[2].data() + fields[2].size(), this->latitude);
      std::from_chars(fields[4].data(), fields[4].data() + fields[4].size(),  this->longitude);
      if (fields[3] == "S") {
        this->latitude = -this->latitude; // South latitude
      }
      if (fields[5] == "W") {
        this->longitude = -this->longitude; // West longitude
      }
    }
    else{
      fmt::print("[SENSOR][GPS] : No GPS fix. Data is not valid\n");
    }
  }
  else{
    fmt::print("Message has too few fields.");
  }
}

std::vector<float> ULTIMATEGPS::get_data(){
  return {this->latitude, this->longitude, this->altitude};
}


