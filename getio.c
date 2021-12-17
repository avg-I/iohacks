
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <machine/cpufunc.h>

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
	val = inb(port);

	printf("port %#0.4X has value %#0.2X\n", port, val);
	close(fd);
	return 0;
}

