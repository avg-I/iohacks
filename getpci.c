
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <machine/cpufunc.h>

int main(int argc, char** argv)
{
	int fd;
	u_int reg;
	u_int val;

	fd = open("/dev/io", O_RDONLY);
	if (fd < 0) {
		perror("open /dev/io");
		return 1;
	}

	reg = strtoul(argv[1], NULL, 0);
	reg = (reg >> 2) << 2;
	outl(0x0cf8, 0x80003800 | reg);
	val = inl(0x0cfd);

	printf("reg %#0.4X of 0:0:7:0 has value %#010X (upper half)\n", reg, val);
	close(fd);
	return 0;
}

