#include "sensor.h"
#include <iostream>
#include <cerrno>
#include <cstring>
#include <vector>
#include <stdio.h>

#include <iomanip>


// i2c를 위한.. linux kernel 이용하는 거임
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>  // ioctl()
#include <unistd.h> // write, read, close, usleep()
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

#define BMP390_PWRCTRL_ADDR 0x1B // power mode select. sleep(00), normal(11), forced(10,01) + p,t enable check sheet
#define BMP390_PWR_MODE 0x33 // normal, both pressure and temperature enabled

#define BMP390_CMD_ADDR 0x7E

// constants fot compensating data
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
  
  char path[] = "/dev/i2c-1"; // linux i2c device path
  
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

 // read data, and convert data to uint32_t, and return it. 
bool read_status = false;
uint32_t i2c_read(uint8_t slave_addr, uint8_t read_addr, int len){
  
  int fd;
  
  char path[] = "/dev/i2c-1"; // linux i2c device path
  
  //std::vecotr<uint8_t> data_arr[len];
  uint8_t data_arr[3]; // 어차피 데이터 값이 3바이트, 나머지는 다 그 이하임..
  

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
    std::cerr << "[ERROR] : " << strerror(errno) << " : Failed to write slave" << path << std::endl;
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
  
  std::cout << "Reading" << len << "bytes : "
            << std::uppercase         // 대문자 쓰기
            << std::setfill('0')      // 빈자리 0채우기
            << std::setw(6)           // 6자리 고정
            << std::hex               // 16진수
            << result
            << "(" << std::dec << result << ")" << std::endl; // 10진수 출력
  
  read_status = true;
  return result; // sucess
}





// write을 통한 register 지정 없이 read만 하는 함수.. bmp390 normal mode 처럼 알아서 register 위치 맞춰주면 read만 하면 됨.
uint32_t i2c_read_only(uint8_t slave_addr, uint8_t read_addr, int len){

  int fd;
  
  char path[] = "/dev/i2c-1"; // linux i2c device path
  
  //std::vecotr<uint8_t> data_arr[len];
  uint8_t data_arr[3]; // 어차피 데이터 값이 3바이트, 나머지는 다 그 이하임..
  

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
  
  std::cout << "Reading" << len << "bytes : "
            << std::uppercase         // 대문자 쓰기
            << std::setfill('0')      // 빈자리 0채우기
            << std::setw(6)           // 6자리 고정
            << std::hex               // 16진수
            << result
            << "(" << std::dec << result << ")" << std::endl; // 10진수 출력
  
  read_status = true;
  return result; // sucess

}





/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////


BMP 390 member functions


///////////////////////////////////////////////////////////////////////////////////////////////////////////// */



// Most Vexing Parse error.. 생성자는 기본 쓰거나 무조건 인자 받도록 하자.. default 인자 ㄴㄴ. init에서 처리하자.
// BMP390::BMP390(){}

// BMP3_ADDR_I2C_PRIM    UINT8_C(0x76)
// BMP3_ADDR_I2C_SEC     UINT8_C(0x77)
bool BMP390::init(){
  
  // I2c slave address set 
  this->slave_addr = BMP390_I2C_ADDR;
  
  // soft reset
  std::cout << "Sensor soft resetting...." << std::endl;
  this->reset();
  usleep(2000000);
  std::cout << "Sensor Soft reset complete" << std::endl;
  

  ///////////////////// SENSOR MODE SETTING ///////////////////////////////////////////////////////////////////
  // pwr mode setting (00110011 = normal mode, both pressure, temperature enabled)
  i2c_write(this->slave_addr, BMP390_PWRCTRL_ADDR, BMP390_PWR_MODE);
  std::cout << "Power mode setting complete" << std::endl;
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

  std::cout << "BMP390 initialized" << std::endl;
  return true;
}




void BMP390::read_data(){
  // read pressure
  this->pressure = i2c_read(this->slave_addr, BMP390_PRESSURE_ADDR_2, 3); 
  
  i2c_read(this->slave_addr, 0x03, 1);
  
  /* parse, compensate 추가
   * this->pressure_data, barometer_data에 대입*/
  if (read_status){
    this->compensate_data();
  }
  else{
    std::cout << "Failed to read" << std::endl;
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


//virtual BMP390::~BMP390(){}

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






