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
	const int slave = 0x49 << 1;

	int reg;
	int val;
	int oldval;

	smb_dev = open(smb_dev_name, 000);
	if (smb_dev < 0) {
		perror("open smb dev");
		return 1;
	}


	reg = strtoul(argv[1], NULL, 0);
	val = strtoul(argv[2], NULL, 0);
	oldval = smbioctl_readB(slave, reg);
	printf("setting reg %u(%#02x) from %u(%#02x) to %u(%#02x)\n", reg, reg, oldval, oldval, val, val);
	getchar();
	smbioctl_writeB(slave, reg, val);


	close(smb_dev);
	return 0;
}

