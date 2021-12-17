
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
	static const int dev_en = 0x30;

	int fd;
	u_int port;
	u_char val;

	fd = open("/dev/io", O_RDONLY);
	if (fd < 0) {
		perror("open /dev/io");
		return 1;
	}

	cfg_entr();

	devsel(0x07);
	val = getreg(0xef);
	val &= ~1;
	setreg(0xef, val);
	val = getreg(0xef);
	printf("LDN7[EF] = %#04x\n", val);

	cfg_exit();

	close(fd);
	return 0;
}

