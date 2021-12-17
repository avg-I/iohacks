
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

	/* Pin 57 is connected to reset,
	 * can be driven by GP12
	 * (has WDTO alternative function)
	 */
	/*setreg(0x2a, 0x80);*/
	/* Pin 103 is connected to Power LED,
	 * can be driven by either PLEDO or GP14.
	 */
	setreg(0x2c, 0x01);


	setreg(dev_sel, 7);

	/* GP12 has WDTO alternative function,
	 * inverted polarity needed for correct
	 * operation
	 */
	/*setreg(0xe2, 0x0A);*/

	/* GP14 inverted polarity lights Power LED */
	setreg(0xe4, 0x02);

	/* Pin 58 (GP13) and Pin 121 (GP17)
	 * do not seem to be meaningfully connected
	 * (both have PLEDO alternative function)
	 */
	setreg(0xe3, 0x01);
	setreg(0xe7, 0x01);

	/* Pin 119 (GP16) and Pin 104 (WDTO)
	 * do not seem to be meaningfully connected
	 * (GP16 has WDTO alternative function)
	 */

	setreg(dev_sel, 8);
	setreg(0xf2, 0x00);
	setreg(0xf3, 0xf9);
	setreg(0xf4, 0xc0);

	outb(super_port, exit_val);
	close(fd);
	return 0;
}

