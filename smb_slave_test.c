#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/endian.h>

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
                perror("ioctl_read");
		fprintf(stderr, "ioctl_read %#x %#x failure\n", slave, addr);
                exit(-1);
        }
        return (ret & 0xFF);
}
static int
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
        return (ret & 0xFFFF);
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
		fprintf(stderr, "ioctl_write %#x %#x %#x failure\n", slave, addr, value);
                exit(-1);
        }
}

#define READ_N_PRINT(slave, addr) {\
	int val = smbioctl_readB((slave), (addr)); \
	if (val > 0 && val < 0xff) \
		printf("read 0x%02X:0x%02X: 0x%02X (%d)\n", (slave), (addr), val, val); \
}

int main()
{
	//static const int slave1 = 0xa0;
	//static const int slave2 = 0xa2;
	static const int slave1 = 0x5a;
	static const int slave2 = 0x92;
	int i;
	int val;
	
	smb_dev = open(smb_dev_name, 000);
	if (smb_dev < 0) {
		perror("open smb dev");
		return 1;
	}

	for (i = 0; i < 10000; ++i)
	{
		//printf("i = %d\n", i);
		val = smbioctl_readB(slave1, 0x27);
		//printf("val = %d\n", val);
		val = smbioctl_readW(slave2, 0x00);
		//val = bswap16(val);
		//val >>= 7;
		//printf("val = %f\n", val / 2.0);
		val = smbioctl_readB(slave1, 0x2a);
		//printf("val = %d\n", val);
	}

	close(smb_dev);
	return 0;
}

