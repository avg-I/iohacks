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
                perror("ioctl_read");
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

static int
athlon_vid2mv(uint8_t vid)
{
	return vid == 0x1f ? 0 : 1850 - vid * 25;
}

#define READ_N_PRINT(slave, addr) {\
	int val = smbioctl_readB((slave), (addr)); \
	if (val > 0 && val < 0xff) \
		printf("read 0x%02X:0x%02X: 0x%02X (%d)\n", (slave), (addr), val, val); \
}

int main(int argc, char** argv)
{
	unsigned int slave;
	int i;

	slave = strtoul(argv[1], NULL, 0);
	if (slave == 0 || (slave & 0x1)) {
		fprintf(stderr, "incorrect slave %u\n", slave);
	}

	smb_dev = open(smb_dev_name, 000);
	if (smb_dev < 0) {
		perror("open smb dev");
		return 1;
	}

	for (i = 0; i <= 0xff; ++i)
	{
		READ_N_PRINT(slave, i);
	}
	return 0;

	close(smb_dev);
	return 0;
}

