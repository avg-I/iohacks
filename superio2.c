
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <machine/cpufunc.h>


static const int super_port = 0x3f0;


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

int main(int argc, char** argv)
{
	static const int dev_sel = 0x7;
	static const int dev_en = 0x30;
	static const uint8_t enter_val = 0x87;
	static const uint8_t exit_val = 0xaa;

	int fd;
	u_int port;
	u_char val;

	fd = open("/dev/io", O_RDONLY);
	if (fd < 0) {
		perror("open /dev/io");
		return 1;
	}

	outb(super_port, enter_val);
	outb(super_port, enter_val);


	setreg(dev_sel, 7);
	setreg(dev_en, 0);
	setreg(0x60, 0x2);
	setreg(0x61, 0x50);
	setreg(dev_en, 1);

	setreg(dev_sel, 8);
	setreg(dev_en, 0);
	setreg(0x60, 0x2);
	setreg(0x61, 0x54);
	setreg(dev_en, 1);

	outb(super_port, exit_val);
	close(fd);
	return 0;
}

