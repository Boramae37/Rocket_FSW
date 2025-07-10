#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

const int bus = 1;
const unsigned long slave_addr = 0x77;
const unsigned short sub_addr = 0x07;


int main()
{
	int fd;
	int rval;
	char path[20];
	unsigned char buf[2];
	unsigned char rxd[3];

	sprintf(path, "/dev/i2c-%d", bus);

	fd = open(path, O_RDWR);

	if ((fd < 0) && (errno == ENOENT)) {
		sprintf(path, "/dev/i2c-%d", bus);

		fd = open(path, O_RDWR);
	}

	if (fd < 0) {
		fprintf(stderr, "Can't open file '/dev/i2c-%d' or '/dev/i2c/%d': %s\r\n", bus, bus, strerror(errno));

		exit(EXIT_FAILURE);
	}

	// Read 1byte
	ioctl(fd, I2C_SLAVE, slave_addr);

	buf[0] = (sub_addr & 0xFF00) >> 8;
	buf[1] = sub_addr & 0xFF;

	if ((rval = write(fd, buf, 2)) < 0) {
		fprintf(stderr, "Writing error! (%lX): %s\r\n", slave_addr, strerror(errno));

		exit(EXIT_FAILURE);
	}

	if ((rval = read(fd, rxd, sizeof(unsigned char)*3)) < 0) {
		fprintf(stderr, "Reading error! (%lX): %s\r\n", slave_addr, strerror(errno));

		exit(EXIT_FAILURE);
	}

  unsigned int result = ((unsigned int)rxd[0] << 16) | ((unsigned int)rxd[1] << 8) | ((unsigned int)rxd[2]);
  unsigned int result2 = ((unsigned int)rxd[2] << 16) | ((unsigned int)rxd[1] << 8) | ((unsigned int)rxd[0]);

  printf("Combined 3 bytes = 0x%06X (%u)\n", result, result);
  printf("Combined 3 bytes = 0x%06X (%u)\n", result2, result2);
  close(fd);

	return 0;
}
