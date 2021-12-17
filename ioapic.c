#include <stdio.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/mman.h>

static const off_t addr = 0xfec00000;
static const size_t length = 0x50;
static const int rdt_offset = 0x10;
static const int rdt_count = 24;

uint64_t rdt_entry(volatile uint32_t *idx, volatile uint32_t *dat, int i)
{
	uint64_t val;

	*idx = rdt_offset + 2 * i + 1;
	val = *dat;
	val <<= 32;
	*idx = rdt_offset + 2 * i;
	val |= *dat;
	return (val);
}

int main()
{
	volatile uint32_t *dat;
	volatile uint32_t *idx;
	volatile void *v;
	uint64_t rdte;
	int fd;
	int i;

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

	for (i = 0; i < rdt_count; i++) {
		rdte = rdt_entry(idx, dat, i);
		printf("%02d 0x%016jx\n", i, (uintmax_t)rdte);
	}
	return (0);
}
