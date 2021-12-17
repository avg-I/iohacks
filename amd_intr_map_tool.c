
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <machine/cpufunc.h>

static const u_int addr_port = 0xc00;
static const u_int data_port = 0xc01;

typedef enum {
	RD_MODE,
	WR_MODE,
} opmode_t;

int main(int argc, char** argv)
{
	char *endp;
	unsigned long x;
	int fd;
	int ch;
	opmode_t mode = RD_MODE;
	uint8_t idx;
	uint8_t val;

	while ((ch = getopt(argc, argv, "rw")) != -1) {
		switch (ch) {
		case 'r':
			mode = RD_MODE;
			break;
		case 'w':
			mode = WR_MODE;
			break;
		default:
			fprintf(stderr, "unexpected option <%c>\n", optopt);
			return (1);
		}
	}

	argv += optind;
	argc -= optind;

	if (argc < 1) {
		fprintf(stderr, "missing index argument\n");
		return (1);
	}
	x = strtoul(argv[0], &endp, 0);
	if (endp == argv[0] || *endp != '\0') {
		fprintf(stderr, "failed to parse index %s\n", argv[0]);
		return (1);
	}
	if (x > UINT8_MAX) {
		fprintf(stderr, "index %lu is out of range\n", x);
		return (1);
	}
	idx = x;

	if (mode == WR_MODE) {
		if (argc < 2) {
			fprintf(stderr, "missing value argument\n");
			return (1);
		}
		x = strtoul(argv[1], &endp, 0);
		if (endp == argv[1] || *endp != '\0') {
			fprintf(stderr, "failed to parse value %s\n", argv[0]);
			return (1);
		}
		if (x > UINT8_MAX) {
			fprintf(stderr, "value %lu is out of range\n", x);
			return (1);
		}
		val = x;
	}

	fd = open("/dev/io", O_RDONLY);
	if (fd < 0) {
		perror("open /dev/io");
		return (1);
	}

	outb(addr_port, idx);
	if (mode == RD_MODE) {
		val = inb(data_port);
		printf("idx 0x%02x has value 0x%02x\n", idx, val);
	} else {
		outb(data_port, val);
	}
	close(fd);
	return (0);
}

