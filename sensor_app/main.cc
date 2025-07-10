#include <iostream>

// i2c를 위한.. linux kernel 이용하는 거임
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>  // ioctl()
#include <fcntl.h>      // open(), O_RDWR
#include <unistd.h>     // read(), write(), close()

#include "bmp3.h"
#include "bmp3_defs.h"


int read_i2c(){
    
  const char *dev_path = "/dev/i2c-1";
  int addr = 0x77; // 센서 주소

  int fd = open(dev_path, O_RDWR); // O_RDWR 은 읽고 쓰기 모드.. Read / wirte only는 설정은 가능하지만, i1c에선 안됨. 읽을 때도 원하는 레지스터 주소를 write해야해서..
  if (fd < -1) {
    perror("I2C open 실패");
    return 0;
  }

  if (ioctl(fd, I2C_SLAVE, addr) < 0) {
    perror("ioctl 실패");
    close(fd);
    return 0;
  }

  
  unsigned char res[3];

  
  unsigned char register_addr[3] = {0x07, 0x08, 0x09};
  for (int i=0; i < 3; i++){
    // 이거 불러줘 명령(wirte) 
    if (write(fd, res, 1) != 1) {
      perror("레지스터 read 실패");
      close(fd);
      return 0;
    } 
  
    // reading
    if(read(fd, res+i, 1) != 1){
      perror("Failed to read data");
      close(fd);
      return errno;
    }

    std::cout << "Read data : 0x" << res[i] << std::endl;
  }
  
}





int main(){
  std::cout << "this is sensor_application" << std::endl; 
  
  int is_read = 1;

  is_read = read_i2c();
  
  std::cout << is_read << std::endl;
  return 0;
}
