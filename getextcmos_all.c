
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

	for (reg = 0; reg < 0x80; reg++)
	{
		outb(0x72, reg | 0x80);
		val = inb(0x73);

		printf("%c", val);
	}
	close(fd);
	return 0;
}

