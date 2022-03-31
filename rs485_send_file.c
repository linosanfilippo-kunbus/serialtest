#include <linux/serial.h>

/* Include definition for RS485 ioctls: TIOCGRS485 and TIOCSRS485 */
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


int configure_serial(int fd, int speed, int parity)
{
        struct termios tty;

        if (tcgetattr (fd, &tty) != 0) {
                perror("tcgetattr")
                return -1;
        }

        cfsetospeed(&tty, speed);
        cfsetispeed(&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

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

int main(int argc, char *argv[])
{
	struct serial_rs485 rs485conf;
	char buff[4096];
	char *prgname;
	char *serdev;
	char *filename;
	int filefd;
	int ttyfd;
	int ttyret;
	int fileret;

	prgname = argv[0];

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <FILE_TO_SEND> <SERIAL_DEVICE>\n", prgname);
		return -1;
	}

	filename = argv[1];
	serdev = argv[2];

	filefd = open(filename, O_RDONLY);
	if (filefd < 0) {
		perror("open file");
		return -1;
	}

	/* Open your specific device (e.g., /dev/mydevice): */
	ttyfd = open (serdev, O_RDWR | O_NOCTTY);
	if (ttyfd < 0) {
		perror("open");
		return -1;
	}

#if 0
	ret = configure_serial(fd);
	if (ret) {
		fprintf(stderr, "failed to configure serial device\n");
		return -1;
	}
#endif

	memset(&rs485conf, 0, sizeof(rs485conf));

	/* Enable RS485 mode: */
	rs485conf.flags |= SER_RS485_ENABLED;
	/* Set logical level for RTS pin equal to 1 when sending: */
	rs485conf.flags |= SER_RS485_RTS_ON_SEND;
	rs485conf.flags &= ~(SER_RS485_RTS_AFTER_SEND);

	rs485conf.delay_rts_before_send = 0; 
	rs485conf.delay_rts_after_send = 0;
#if 0
	rs485conf.flags |= SER_RS485_RX_DURING_TX;
#endif

	ttyret = ioctl(ttyfd, TIOCSRS485, &rs485conf);
	if (ttyret) {
		perror("ioctl tty");
		goto out;
	}

	fprintf(stderr, "Sending file ...\n");

	do {
		fileret = read(filefd, buff, sizeof(buff));
		if (fileret > 0) {
			fprintf(stderr, "writing %u bytes to tty\n", fileret);
			ttyret = write(ttyfd, buff, fileret);
			if (ttyret != fileret) {
				fprintf(stderr, "got ttyret = %u, while filret was %u\n",
					ttyret, fileret);
			}
		} else if (fileret == 0)
			fprintf(stderr, "no more data to read\n");
	} while ((fileret > 0) && (ttyret > 0));

	if (fileret < 0)
		perror("read from file");

	if (ttyret < 0)
		perror("write to tty");

	if (!(ttyret | fileret))
		goto out;
	
	fprintf(stderr, "all data sent\n");

out:
	/* Close the devices when finished: */
	close(ttyfd);
	close(filefd);

	if (ttyret < 0 || fileret < 0)
		return -1;

	return 0;
}
