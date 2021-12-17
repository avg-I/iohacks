
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

	fprintf(stderr, "going to read\n");
	sleep(5);

	for (reg = 14; reg < 0x80; reg++)
	{
		//outb(0x70, reg | 0x80);
		outb(0x70, reg);
		val = inb(0x71);

		printf("%c", val);
		fprintf(stderr, "index 0x%02x\n", reg);
	}
	close(fd);
	return 0;
}

