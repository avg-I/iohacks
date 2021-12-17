#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <dev/smbus/smb.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

static const char smb_dev_name[] = "/dev/smb0";
static int smb_dev;

static int
smbioctl_readB(int slave, int addr)
{
        struct smbcmd cmd;
        char ret;
        cmd.slave = slave;
        cmd.cmd = addr;
        cmd.data.byte_ptr = &ret;
        if (ioctl(smb_dev, SMB_READB, &cmd) == -1) {
                return(-1);
        }
        return (ret & 0xFF);
}

static void
smbioctl_writeB(int slave, int addr, int value)
{
        struct smbcmd cmd;
        cmd.slave = slave;
        cmd.cmd = addr; 
        cmd.data.byte = value;
        if (ioctl(smb_dev, SMB_WRITEB, &cmd) == -1) {
                perror("ioctl_write");
                exit(-1);
        }
}


#define READ_N_PRINT(slave, addr) {\
	int val = smbioctl_readB((slave), (addr)); \
	if (val >= 0) \
		printf("read 0x%02X:0x%02X: 0x%02X (%d)\n", (slave), (addr), val, val); \
}

int main(int argc, char** argv)
{
	const int slave = 0x2d << 1;
	const int pwm1_addr = 0x5a;
	const int pwm1sel_addr = 0x5c;
	const int pwm1sel_mask = 0x08;
	const int pwm2sel_addr = 0x4c;
	const int pwm2sel_mask = 0x80;
	const int pwm_clk_addr = pwm1sel_addr;
	const int pwm2_clk_mask = 0x07;

	int addr;
	int val;
	int i;

	smb_dev = open(smb_dev_name, 000);
	if (smb_dev < 0) {
		perror("open smb dev");
		return 1;
	}


	val = smbioctl_readB(slave, pwm1_addr);
	printf("changing pwm1 from %u(%02x)", val, val);

	val = atoi(argv[1]);
	printf(" to %u(%02x)\n", val, val);
	smbioctl_writeB(slave, pwm1_addr, val);

	val = smbioctl_readB(slave, pwm1_addr);
	printf("\tnew value verification %u(%02x)\n", val, val);


	close(smb_dev);
	return 0;
}

