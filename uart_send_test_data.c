#include <linux/serial.h>

/* Include definition for RS485 ioctls: TIOCGRS485 and TIOCSRS485 */
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>


#if 0
int configure_serial(int fd, int speed, int parity)
{
        struct termios tty;

        if (tcgetattr (fd, &tty) != 0) {
                perror("tcgetattr")
                return -1;
        }


	// convert break to null byte, no CR to NL translation,
	// no NL to CR translation, don't mark parity errors or breaks
	// no input parity check, don't strip high bit off,
	// no XON/XOFF software flow control
	//
	config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
			    INLCR | PARMRK | INPCK | ISTRIP | IXON);

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
	//
	// Output flags - Turn off output processing
	//
	// no CR to NL translation, no NL to CR-NL translation,
	// no NL to CR translation, no column 0 CR suppression,
	// no Ctrl-D suppression, no fill characters, no case mapping,
	// no local output processing
	//
	// config.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
	//                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
        tty.c_oflag = 0;


        cfsetospeed(&tty, B115200);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout


        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0) {
                error_message ("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}
#endif

static unsigned char get_crc(unsigned char *buff, unsigned int len)
{
	unsigned char ret = 0;

	while (len--)
		ret = ret ^ buff[len];
	return ret;
}

int main(int argc, char *argv[])
{
	//char testdata[] = {0xa1,0x6,0xff,0xff,0xff,0xff,0x49,0x3};
	unsigned char testdata[] = { 0x9d, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00 };
//	unsigned char testdata[] = { 'h', 'a', 'l', 'l', 'o' };

	struct timespec ts;
	unsigned char crc;
	char *prgname;
	char *serdev;
	int written;
	int ttyfd;
	int ret = -1;

	prgname = argv[0];

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <SERIAL_DEVICE>\n", prgname);
		return -1;
	}

	serdev = argv[1];

	/* Open your specific device (e.g., /dev/mydevice): */
	ttyfd = open(serdev, O_WRONLY| O_NOCTTY);
	if (ttyfd < 0) {
		perror("open");
		return -1;
	}

	if (!isatty(ttyfd)) {
		perror("no tty\n");
		goto out;
	} 

#if 0
	ret = configure_serial(fd);
	if (ret) {
		fprintf(stderr, "failed to configure serial device\n");
		return -1;
	}
#endif
	
	ts.tv_sec = 0;
	ts.tv_nsec = 1000000;

	fprintf(stderr, "Sending data...\n");
	do {
		written = write(ttyfd, testdata, sizeof(testdata));
		if (written != sizeof(testdata)) {
			fprintf(stderr, "failed to write all testdata: %s\n", strerror(errno));
			break;
		}

		crc = get_crc(testdata, sizeof(testdata));

		written = write(ttyfd, &crc, 1);
		if (written != 1) {
			fprintf(stderr, "failed to write crc: %s\n", strerror(errno));
			break;
		}
			
	
		if (nanosleep(&ts, NULL))
			perror("nanosleep");
	} while (1);

out:
	ret = 0;
	/* Close the devices when finished: */
	close(ttyfd);

	return ret;
}
