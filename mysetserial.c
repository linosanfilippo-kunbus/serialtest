
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <linux/serial_core.h>


int main(int argc, char **argv)
{
	struct serial_struct ser;
	int fd;
	int ret;

	memset(&ser, 0, sizeof(ser));

	ser.type = PORT_AMBA;
	ser.baud_base = 9600;
	ser.iomem_base = (void *) (unsigned long) 0xFe201100;

	fd = open("/dev/ttyAMA0", O_RDWR);

	if (fd < 0) {
		perror("open");
		return -1;
	}
	
	ret = ioctl(fd, TIOCSSERIAL, &ser);

	if (ret)
		fprintf(stderr, "GOT ioctl error: %i\n", ret);
	else
		fprintf(stderr, "IOCTL OK\n");


	return 0;
}
	
