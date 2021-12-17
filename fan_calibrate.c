#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <dev/smbus/smb.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

static const char SMB_DEV_NAME[] = "/dev/smb0";
const int SLAVE = 0x2d << 1;
const int AUX_SLAVE1 = 0x48 << 1;
const int AUX_SLAVE2 = 0x49 << 1;
static int smb_dev;

static uint8_t
smbioctl_readB(uint8_t slave, uint8_t addr)
{
        struct smbcmd cmd;
        uint8_t ret;
        cmd.slave = slave;
        cmd.cmd = addr;
        cmd.data.byte_ptr = &ret;
        if (ioctl(smb_dev, SMB_READB, &cmd) == -1) {
		fprintf(stderr, "ioctl_read %#x %#x failure\n", slave, addr);
                perror("ioctl_read");
                exit(-1);
        }
	//printf("ioctl_read %#x %#x %#x\n", slave, addr, ret);
        return ret;
}

static uint16_t
smbioctl_readW(int slave, int addr)
{
        struct smbcmd cmd;
        short ret;
        cmd.slave = slave;
        cmd.cmd = addr;
        cmd.data.word_ptr = &ret;
        if (ioctl(smb_dev, SMB_READW, &cmd) == -1) {
                perror("ioctl_readW");
		fprintf(stderr, "ioctl_readW %#x %#x failure\n", slave, addr);
                exit(-1);
        }
	//printf("ioctl_read %#x %#x %#x\n", slave, addr, ret);
        return (ret & 0xFFFF);
}

static void
smbioctl_writeB(uint8_t slave, uint8_t addr, uint8_t value)
{
        struct smbcmd cmd;
        cmd.slave = slave;
        cmd.cmd = addr; 
        cmd.data.byte = value;
        if (ioctl(smb_dev, SMB_WRITEB, &cmd) == -1) {
		fprintf(stderr, "ioctl_write %#x %#x %#x failure\n", slave, addr, value);
                perror("ioctl_write");
                exit(-1);
        }
	//printf("ioctl_write %#x %#x %#x\n", slave, addr, value);
}

static uint8_t get_t1()
{
	static const uint8_t reg = 0x27;
	return smbioctl_readB(SLAVE, reg);
}
static uint8_t get_t2()
{
	static const uint8_t reg = 0x00;
	return smbioctl_readW(AUX_SLAVE2, reg) & 0xFF;
}
static unsigned int to_rpm(uint8_t val, uint8_t div)
{
	unsigned int res;

	if (val == 0 || val == 0xff)
		return 0;
	res = (1350000.0 / (1 << div)) / val;
	if (res > 20000)
		return 0;
	return res;
}


static const int SHORT_SLEEP = 2;
static const int LONG_SLEEP = 10;

struct fan {
	uint8_t pwm;
	uint8_t rpm;
	uint8_t div;
	int     divno;
	const char *name;
} fans[] = {
	{ 0x5b, 0x28, 0, 1, "chassis (pwm1)"},
	{ 0x5f, 0x2a, 0, 3, "cpu (pwm4)"},
#ifdef notyet
	{ 0x5e, 0x29, 0, 2, "psu (pwm3)"},
#endif
};

#define N_ELEMENTS(arr)		sizeof(arr)/sizeof(arr[0])

int main(int argc, char** argv)
{

	int i;

	smb_dev = open(SMB_DEV_NAME, 000);
	if (smb_dev < 0) {
		perror("open smb dev");
		return 1;
	}

	/* get divisors */
	/* XXX they should probably be dynamically adapted throughout
	 * the calibration.
	 */
	uint8_t cr5d = smbioctl_readB(SLAVE, 0x5d);
	uint8_t cr47 = smbioctl_readB(SLAVE, 0x47);
	uint8_t cr4b = smbioctl_readB(SLAVE, 0x4b);

	/*
	 * Divisors:
	 * 1: 0x5d:5, 0x47:5_4 
	 * 2: 0x5d:6, 0x47:7_6 
	 * 3: 0x5d:7, 0x4b:7_6 
	 */
	for (i = 0; i < N_ELEMENTS(fans); ++i) {
		uint8_t div;

		switch (fans[i].divno) {
		case 1:
			div = ((cr5d >> 3) & 0x4) | ((cr47 >> 4) & 0x3);
			break;
		case 2:
			div = ((cr5d >> 4) & 0x4) | ((cr47 >> 6) & 0x3);
			break;
		case 3:
			div = ((cr5d >> 5) & 0x4) | ((cr4b >> 6) & 0x3);
			break;
		default:
			fprintf(stderr, "unknown fan/divisor number %s (%u)\n", fans[i].name, fans[i].divno);
			return 1;
		}

		fans[i].div = div;
		printf("div = %u (%u) for fan %s\n", div, 1 << div, fans[i].name);
		fflush(stdout);
	}

	/* calibration */
	for (i = 0; i < N_ELEMENTS(fans); ++i) {
		int j;

		/* full speed */
		smbioctl_writeB(SLAVE, fans[i].pwm, 0xff);
		sleep(LONG_SLEEP);

		for (j = 0xff; j >= 0; j--) {
			smbioctl_writeB(SLAVE, fans[i].pwm, j);
			sleep(SHORT_SLEEP);

			uint8_t raw_rpm = smbioctl_readB(SLAVE, fans[i].rpm);
			unsigned int rpm = to_rpm(raw_rpm, fans[i].div);
			uint8_t t2 = get_t2();
			uint8_t t1 = get_t1();

			printf("%u\t%u\t%u\t%u\t%u\n", j, raw_rpm, rpm, t1, t2);
			fflush(stdout);
		}

		sleep(LONG_SLEEP);

		for (j = 0; j <= 0xff; j++) {
			smbioctl_writeB(SLAVE, fans[i].pwm, j);
			sleep(SHORT_SLEEP);

			uint8_t raw_rpm = smbioctl_readB(SLAVE, fans[i].rpm);
			unsigned int rpm = to_rpm(raw_rpm, fans[i].div);
			uint8_t t2 = get_t2();
			uint8_t t1 = get_t1();

			printf("%u\t%u\t%u\t%u\t%u\n", j, raw_rpm, rpm, t1, t2);
			fflush(stdout);
		}
	}
	
	
	close(smb_dev);
	return 0;
}

