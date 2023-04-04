
#include "pibridge_simulation.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


static int setup_response(unsigned int addr, unsigned char *buff, unsigned int len)
{
	int ret = -1;

	if (addr == 29) {
		if (len < 20)
			return -1;
		buff[0] = (addr | 0x80);
		buff[1] = 17;
		memset(&buff[2], 0, 17);
		buff[19] = get_crc(buff, 19);

		ret = 20;
	}

	return ret;
}

static int send_response(int ttyfd, unsigned int addr)
{
	unsigned char buff[32];
	int ret;
	int len;

	len = setup_response(addr, buff, sizeof(buff));

	if (len < 0)
		return len;

	ret = write(ttyfd, buff, len);

	if (ret < 0) {
		perror("write response");
		return ret;
	}

	return 0;
}


static int read_request(int ttyfd, uint8_t *addr)
{
	const unsigned int HDR_SIZE = 2;
	unsigned char buff[64];
	unsigned int to_read;
	unsigned int bytes_read;
	unsigned int datalen;
	uint8_t crc_exp;
	uint16_t hdr;
	uint8_t crc;
	bool invalid_len = false;
	int ret;

	// read header
	to_read = HDR_SIZE;
	bytes_read = 0;

	do {
		ret = read(ttyfd, buff + bytes_read, to_read);
		if (ret < 0) {
			perror("read");
			break;
		}
		//fprintf(stderr, "read %i bytes\n", ret);
		bytes_read += ret;
		to_read -= ret;
	} while (to_read);

	if (ret < 0)
		return ret;

	hdr = buff[0];
	hdr |= (buff[1] << 8);

	//fprintf(stderr, "got header: 0x%04x\n", hdr);
	datalen = PB_LEN(hdr);
	//fprintf(stderr, "got datalen: %u\n", datalen);
	*addr = PB_ADDR(hdr);
	//fprintf(stderr, "got addr: %u\n", *addr);

	if (!datalen) {
		fprintf(stderr, "got invalid datalen 0, dumpin next 20 bytes\n");
		datalen = 20;
		invalid_len = true;
	}

	// read data
	to_read = datalen + 1;
	bytes_read = 0;

	do {
		ret = read(ttyfd, buff + HDR_SIZE + bytes_read, to_read);
		if (ret < 0) {
			perror("read");
			break;
		}
		//fprintf(stderr, "read %i bytes\n", ret);
		bytes_read += ret;
		to_read -= ret;
	} while (to_read);

	if (ret < 0) {
		fprintf(stderr, "failed to read request, dumping data\n");
		dump_buffer(buff, HDR_SIZE + bytes_read);
		return ret;
	}

	if (invalid_len) {
		fprintf(stderr, "dumping invalid len data\n");
		dump_buffer(buff, HDR_SIZE + bytes_read);
	}

	crc = buff[HDR_SIZE + bytes_read - 1];
	crc_exp = get_crc(buff, HDR_SIZE + datalen);
	//fprintf(stderr, "GOT EXP CRC: 0x%02x, READING CRC\n", crc_exp);
	// read CRC
	if (crc_exp != crc) {
		fprintf(stderr, "GOT wrong crc (0x%02x, exp: 0x%02x\n", crc, crc_exp);
		dump_buffer(buff, HDR_SIZE + datalen + 1);
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char *prgname;
	char *serdev;
	int ttyfd;
	uint8_t addr;
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

	//fprintf(stderr, "Waiting for data...\n");

	do {
		ret = read_request(ttyfd, &addr);
		if (ret) {
			fprintf(stderr, "failed to read request: %i\n", ret);
			break;
		}

		ret = send_response(ttyfd, addr);
		if (ret < 0)
			fprintf(stderr, "failed to send response: %i\n", ret);
	} while (1);
out:
	ret = 0;
	/* Close the devices when finished: */
	close(ttyfd);

	return ret;
}

