
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#include <dev/superio/superio_io.h>

static const char *dev = "/dev/superio0";

int main(int argc, char** argv)
{
	struct superiocmd cmd;
	char *endp;
	unsigned long req;
	unsigned long x;
	int fd;
	int ch;

	req = SUPERIO_CR_READ;
	while ((ch = getopt(argc, argv, "rw")) != -1) {
		switch (ch) {
		case 'r':
			req = SUPERIO_CR_READ;
			break;
		case 'w':
			req = SUPERIO_CR_WRITE;
			break;
		default:
			fprintf(stderr, "unexpected option <%c>\n", optopt);
			return (1);
		}
	}

	argv += optind;
	argc -= optind;

	if (argc < 1) {
		fprintf(stderr, "missing LDN argument\n");
		return (1);
	}
	x = strtoul(argv[0], &endp, 0);
	if (endp == argv[0] || *endp != '\0') {
		fprintf(stderr, "failed to parse LDN %s\n", argv[0]);
		return (1);
	}
	if (x > UINT8_MAX) {
		fprintf(stderr, "LDN %lu is out of range\n", x);
		return (1);
	}
	cmd.ldn = x;

	if (argc < 2) {
		fprintf(stderr, "missing CR argument\n");
		return (1);
	}
	x = strtoul(argv[1], &endp, 0);
	if (endp == argv[1] || *endp != '\0') {
		fprintf(stderr, "failed to parse CR %s\n", argv[0]);
		return (1);
	}
	if (x > UINT8_MAX) {
		fprintf(stderr, "CR %lu is out of range\n", x);
		return (1);
	}
	cmd.cr = x;

	if (req == SUPERIO_CR_WRITE) {
		if (argc < 3) {
			fprintf(stderr, "missing value argument\n");
			return (1);
		}
		x = strtoul(argv[2], &endp, 0);
		if (endp == argv[2] || *endp != '\0') {
			fprintf(stderr, "failed to parse value %s\n", argv[0]);
			return (1);
		}
		if (x > UINT8_MAX) {
			fprintf(stderr, "value %lu is out of range\n", x);
			return (1);
		}
		cmd.val = x;
	}

	if ((fd = open(dev, O_RDWR)) == -1) {
		perror(dev);
		return (1);
	}

	if (ioctl(fd, req, &cmd) == -1) {
		perror("ioctl");
		return (1);
	}

	if (req == SUPERIO_CR_READ) {
		printf("LDN 0x%02x CR 0x%02x has value 0x%02x\n",
		    cmd.ldn, cmd.cr, cmd.val);
	}

	(void)close(fd);
	return (0);
}

