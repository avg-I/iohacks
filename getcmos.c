
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <machine/cpufunc.h>

int main(int argc, char** argv)
{
	int fd;
	u_int reg;
	u_char val;

	fd = open("/dev/io", O_RDONLY);
	if (fd < 0) {
		perror("open /dev/io");
		return 1;
	}

	reg = strtoul(argv[1], NULL, 0);

	outb(0x70, reg);
	val = inb(0x71);

	printf("reg %#0.4X of cmos has value %#0.2X\n", reg, val);
	close(fd);
	return 0;
}

