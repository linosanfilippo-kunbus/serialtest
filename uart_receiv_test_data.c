
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#define TO_RECEIVE 20

static unsigned char get_crc(unsigned char *buff, unsigned int len)
{
	unsigned char ret = 0;

	while (len--)
		ret = ret ^ buff[len];
	return ret;
}


static int check_test_data(unsigned char *buff, unsigned int len)
{
	unsigned char crc;

	crc = get_crc(buff, len - 1);

	if (crc != buff[len - 1]) {
		fprintf(stderr, "invalid crc (got: 0x%02x, exp: 0x%02x)\n",
			buff[len - 1], crc);
		return -1;
	}
	
	return 0;
}

int main(int argc, char *argv[])
{
	unsigned int to_receive;
	unsigned char rcvbuff[4096];
	unsigned long counter;
	int data_read;
	char *prgname;
	char *serdev;
	int ttyfd;
	int ret = -1;

	prgname = argv[0];

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <SERIAL_DEVICE>\n", prgname);
		return -1;
	}

	serdev = argv[1];

	/* Open your specific device (e.g., /dev/mydevice): */
	ttyfd = open(serdev, O_RDONLY | O_NOCTTY);
	if (ttyfd < 0) {
		perror("open");
		return -1;
	}

	if (!isatty(ttyfd)) {
		perror("no tty\n");
		goto out;
	} 

	to_receive = TO_RECEIVE;
	data_read = 0;
	counter = 0;

	fprintf(stderr, "Receiving data...\n");
	do {
		int i;

		ret = read(ttyfd, rcvbuff + data_read, to_receive);

		if (ret < 0) {
			perror("read");
			break;
		}

		to_receive -= ret;
		data_read += ret;

		if (to_receive == 0) {

#if 0
				for (i = 0; i < TO_RECEIVE; i++)
					fprintf(stderr, "Data: 0x%02x\n", rcvbuff[i]);
#endif


			ret = check_test_data(rcvbuff, TO_RECEIVE);

			if (ret) {
				fprintf(stderr, "Error in test data: \n");
				for (i = 0; i < TO_RECEIVE; i++)
					fprintf(stderr, "Data: 0x%02x\n", rcvbuff[i]);
			}

			to_receive = TO_RECEIVE;
			data_read = 0;
			counter++;

			if (counter > 10000) {
				fprintf(stderr, "still receiving...\n");
				counter = 0;
			}
		}
			

	} while (1);

out:
	ret = 0;
	/* Close the devices when finished: */
	close(ttyfd);

	return ret;
}
