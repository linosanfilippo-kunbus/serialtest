//#include <linux/serial.h>

/* Include definition for RS485 ioctls: TIOCGRS485 and TIOCSRS485 */

#include "pibridge_simulation.h"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>


static int read_response(int ttyfd)
{
	const unsigned int HDR_SIZE = 2;
	unsigned char buff[64];
	unsigned int to_read;
	unsigned int bytes_read;
	unsigned int datalen;
	struct timespec ts;
	fd_set fdset;
	uint16_t hdr;
	uint8_t crc_exp;
	uint8_t crc;
	int ret;

	// read header
	to_read = HDR_SIZE;
	bytes_read = 0;

	do {
		FD_ZERO(&fdset);
		FD_SET(ttyfd, &fdset);

		/* 10 ms */
		ts.tv_sec = 0;
		ts.tv_nsec = 10000000;

		//fprintf(stderr, "waiting before select\n");
		ret = pselect(ttyfd + 1, &fdset, NULL, NULL, &ts, NULL);
		if (ret < 0) {
			perror("select");
			break;
		}
		if (ret == 0) {
			fprintf(stderr, "timeout waiting for response\n");
			ret = -1;
			break;
		}

		//fprintf(stderr, "after select\n");

		ret = read(ttyfd, buff + bytes_read, to_read);
		if (ret < 0) {
			perror("read");
			break;
		}
		bytes_read += ret;
		to_read -= ret;
	} while (to_read);

	if (ret < 0)
		return ret;

	hdr = buff[0];
	hdr |= (buff[1] << 8);

//	fprintf(stderr, "GOT HDR: 0x%04x\n", hdr);

	datalen = PB_LEN(hdr);

	if (!datalen || datalen > 32) {
		fprintf(stderr, "got invalid datalen %u\n", datalen);
		return -1;
	}

	// read data
	to_read = datalen + 1;
	bytes_read = 0;

	do {
		FD_ZERO(&fdset);
		FD_SET(ttyfd, &fdset);

		/* 10 ms */
		ts.tv_sec = 0;
		ts.tv_nsec = 10000000;

//		fprintf(stderr, "waiting before select\n");
		ret = pselect(ttyfd + 1, &fdset, NULL, NULL, &ts, NULL);
		if (ret < 0) {
			perror("select");
			break;
		}
		if (ret == 0) {
			fprintf(stderr, "timeout waiting for response\n");
			ret = -1;
			break;
		}

//		fprintf(stderr, "after select\n");

		ret = read(ttyfd, buff + HDR_SIZE + bytes_read, to_read);
		if (ret < 0) {
			perror("read");
			break;
		}
		bytes_read += ret;
		to_read -= ret;
	} while (to_read);

	if (ret < 0)
		return ret;

	crc = buff[HDR_SIZE + bytes_read - 1];
	crc_exp = get_crc(buff, HDR_SIZE + datalen);

	if (crc_exp != crc) {
		fprintf(stderr, "GOT wrong crc (0x%02x, exp: 0x%02x\n", crc, crc_exp);
		return -1;
	}

	return 0;
}


static int send_request(int ttyfd)
{
	unsigned char testdata[] = { 0x1d, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0x00, 0x00     , 0x00};
	int ret;


	testdata[sizeof(testdata) - 1] = get_crc(testdata, sizeof(testdata));
	/* Write request */
	ret = write(ttyfd, testdata, sizeof(testdata));
	if (ret < 0) {
		fprintf(stderr, "failed to write testdata: %s\n", strerror(errno));
		return ret;
	}

	if (ret != sizeof(testdata)) {
		fprintf(stderr, "failed to write all testdata (%u from %u)\n",
			ret, sizeof(testdata));
		return ret;
	}

	return 0;
}


int main(int argc, char *argv[])
{
	unsigned int counter;
	struct timespec ts_start;
	struct timespec ts_end;
	struct timespec ts;
	unsigned int max_latency;
	unsigned int diff;
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
	ttyfd = open(serdev, O_RDWR | O_NOCTTY);
	if (ttyfd < 0) {
		perror("open");
		return -1;
	}

	if (!isatty(ttyfd)) {
		perror("no tty\n");
		goto out;
	}

	//fprintf(stderr, "Sending data...\n");

	counter = 0;
	max_latency = 0;
	diff = 0;

	do {
		clock_gettime(CLOCK_REALTIME, &ts_start);

		ret = send_request(ttyfd);
		if (ret) {
			fprintf(stderr, "failed to send request: %i\n", ret);
			continue;
		}

		ret = read_response(ttyfd);
		if (ret < 0) {
			fprintf(stderr, "failed to get response: %i\n", ret);
			continue;
		}

#if 1
		ts.tv_sec = 0;
		ts.tv_nsec = 1000000;

#if 0
		if (nanosleep(&ts, NULL))
			perror("nanosleep");
#endif

#else
		//fprintf(stderr, "waiting...\n");
		sleep(5);
#endif
		counter++;
		clock_gettime(CLOCK_REALTIME, &ts_end);

		diff = ((ts_end.tv_sec - ts_start.tv_sec) * 1000 * 1000) +
		       ((ts_end.tv_nsec - ts_start.tv_nsec) / 1000);

		if (diff > max_latency) {
			max_latency = diff;
			fprintf(stderr, "max latency is %u\n", max_latency);
		}

		if (counter > 1000) {
			fprintf(stderr, "communicating...\n");
			counter = 0;
		}
	} while (1);
out:
	ret = 0;
	/* Close the devices when finished: */
	close(ttyfd);

	return ret;
}
