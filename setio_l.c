
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <machine/cpufunc.h>

int main(int argc, char** argv)
{
	int fd;
	u_int port;
	uint32_t val;

	fd = open("/dev/io", O_RDONLY);
	if (fd < 0) {
		perror("open /dev/io");
		return 1;
	}

	port = strtoul(argv[1], NULL, 0);
	val = strtoul(argv[2], NULL, 0);
	outl(port, val);

	val = inl(port);

	printf("port %#0.4x now has value %#0.8x\n", port, val);
	close(fd);
	return 0;
}

