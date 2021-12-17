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


#define READ_N_PRINT(slave, addr) {\
	int val = smbioctl_readB((slave), (addr)); \
	printf("%c", val); \
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

	for (i = 0; i <= 0x7f; ++i)
	{
		READ_N_PRINT(slave, i);
	}
	return 0;

	close(smb_dev);
	return 0;
}

