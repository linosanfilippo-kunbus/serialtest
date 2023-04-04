#ifndef _PIBRIDGE_SIMULATION_H
#define _PIBRIDGE_SIMULATION_H

#include <stdio.h>

#define PB_ADDR(hdr) \
	(hdr & 0x3F)

#define PB_TYPE(hdr) \
	((hdr >> 6) & 0x1)

#define PB_RSP(hdr) \
	((hdr >> 7) & 0x1)

#define PB_LEN(hdr) \
	((hdr >> 8) & 0x1F)

#define PB_CMD(hdr) \
	((hdr >> 13) & 0x3)

static inline void dump_buffer(const unsigned char *buff, unsigned int len)
{
	int i;

	for (i = 0; i < len; i++)
		fprintf(stderr, "0x%02x  ", buff[i]);

	fprintf(stderr, "\n");
}

static inline unsigned char get_crc(const unsigned char *buff, unsigned int len)
{
	unsigned char ret = 0;

	while (len--)
		ret = ret ^ buff[len];
	return ret;
}

#endif /* _PIBRIDGE_SIMULATION_H */
