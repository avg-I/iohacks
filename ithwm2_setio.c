
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <machine/cpufunc.h>

static const u_int base = 0x290;

int main(int argc, char** argv)
{
	int fd;
	u_int port;
	u_char val;

	fd = open("/dev/io", O_RDONLY);
	if (fd < 0) {
		perror("open /dev/io");
		return 1;
	}

	port = strtoul(argv[1], NULL, 0);
	val = strtoul(argv[2], NULL, 0);
	outb(base + 5, port);
	outb(base + 6, val);

	outb(base + 5, port);
	val = inb(base + 6);

	printf("port %#0.4X now has value %#0.2X\n", port, val);
	close(fd);
	return 0;
}

