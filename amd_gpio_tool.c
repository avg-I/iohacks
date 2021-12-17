
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

static const off_t gpio_base = 0xfed81500;
static const off_t gpio_size = 0x300;
static const off_t mux_base = 0xfed80d00;

typedef enum {
	RD_MODE,
	WR_MODE,
} opmode_t;

int main(int argc, char** argv)
{
	volatile uint32_t *mem;
	char *endp;
	unsigned long x;
	int fd;
	int ch;
	opmode_t mode = RD_MODE;
	unsigned int off;
	uint32_t val;

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
		fprintf(stderr, "failed to parse offset %s\n", argv[0]);
		return (1);
	}
	if (x + 4 > gpio_size) {
		fprintf(stderr, "offset %lu is out of range\n", x);
		return (1);
	}
	off = x;
	if ((off % 4) != 0) {
		fprintf(stderr, "offset %lu is not on 4-byte boundary\n", x);
		return (1);
	}

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
		if (x > UINT32_MAX) {
			fprintf(stderr, "value %lu is out of range\n", x);
			return (1);
		}
		val = x;
	}

	if ((fd = open("/dev/mem", O_RDWR)) == -1) {
		perror("/dev/mem");
		return (1);
	}

	mem = mmap(NULL, gpio_size,
	    PROT_READ | (mode == WR_MODE ? PROT_WRITE : 0),
	    MAP_SHARED, fd, gpio_base);
	if (mem == MAP_FAILED) {
		perror("mmap");
		return (1);
	}

	if (mode == RD_MODE) {
		val = mem[off / 4];
		printf("offset 0x%02x has value 0x%08x\n", off, val);
	} else {
		mem[off / 4] = val;
	}

	(void)munmap(mem, gpio_size);
	(void)close(fd);
	return (0);
}

