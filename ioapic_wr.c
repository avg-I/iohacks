#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static const off_t addr = 0xfec00000;
static const size_t length = 0x50;
static const int rdt_offset = 0x10;
static const int rdt_count = 24;

void
set_rdt_entry(volatile uint32_t *idx, volatile uint32_t *dat, int i,
    uint64_t val)
{
	*idx = rdt_offset + 2 * i + 1;
	*dat = (uint32_t)(val >> 32);
	*idx = rdt_offset + 2 * i;
	*dat = (uint32_t)val;
}

int main(int argc, char **argv)
{
	volatile uint32_t *dat;
	volatile uint32_t *idx;
	volatile void *v;
	uint64_t val;
	int which;
	int fd;

	if (argc != 3) {
		fprintf(stderr, "two arguments expected\n");
		return (1);
	}
	which = strtoul(argv[1], NULL, 0);
	if (errno != 0) {
		perror("couldn't parse pin number");
		return (1);
	}
	if (which >= rdt_count) {
		fprintf(stderr, "wrong pin number %d\n", which);
		return (1);
	}
	val = strtoumax(argv[2], NULL, 0);
	if (errno != 0) {
		perror("couldn't parse value");
		return (1);
	}

	if((fd = open("/dev/mem", O_RDWR)) == -1) {
		perror("/dev/mem");
		return (1);
	}

	v = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr);
	if (v == MAP_FAILED) {
		perror("mmap");
		return (1);
	}

	idx = v;
	dat = v + 0x10;

	set_rdt_entry(idx, dat, which, val);
	return (0);
}
