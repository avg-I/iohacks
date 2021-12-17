
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <machine/cpufunc.h>

static const u_int addr_port = 0xcd6;
static const u_int data_port = 0xcd7;

int main(int argc, char** argv)
{
	int fd;
	u_char reg;
	u_char val;

	fd = open("/dev/io", O_RDONLY);
	if (fd < 0) {
		perror("open /dev/io");
		return 1;
	}

	reg = strtoul(argv[1], NULL, 0);
	outb(addr_port, reg);
	val = inb(data_port);

	printf("reg %#0.2X has value %#0.2X\n", reg, val);
	close(fd);
	return 0;
}

