
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <machine/cpufunc.h>


static const int super_port = 0x2e;


static void setreg(int reg, uint8_t val)
{
	outb(super_port, reg);
	outb(super_port + 1, val);
}

static uint8_t getreg(int reg)
{
	outb(super_port, reg);
	return inb(super_port + 1);
}

static void cfg_entr()
{
	outb(super_port, 0x87);
	outb(super_port, 0x01);
	outb(super_port, 0x55);
	outb(super_port, 0x55);
}

static void cfg_exit()
{
	outb(super_port, 0x02);
	outb(super_port, 0x02);
}

static void devsel(uint8_t ldn)
{
	setreg(0x07, ldn);
}

int main(int argc, char** argv)
{
	int fd;
	u_char dev;
	u_char reg;
	u_char oldval;

	dev = strtoul(argv[1], NULL, 0);
	reg = strtoul(argv[2], NULL, 0);

	fd = open("/dev/io", O_RDONLY);
	if (fd < 0) {
		perror("open /dev/io");
		return 1;
	}

	cfg_entr();

	devsel(dev);
	oldval = getreg(reg);
	printf("LDN_%x[%02x] = %#04x\n", dev, reg, oldval);

	cfg_exit();

	close(fd);
	return 0;
}

