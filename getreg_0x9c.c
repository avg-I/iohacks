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
smbioctl_readB(int slave)
{
        struct smbcmd cmd;
        cmd.slave = slave;
        cmd.cmd = 0;
        if (ioctl(smb_dev, SMB_RECVB, &cmd) == -1) {
                perror("ioctl_read");
                exit(-1);
        }
        return (cmd.cmd & 0xFF);
}


int main(int argc, char** argv)
{
	const int slave = 0x4e << 1;

	int val;

	smb_dev = open(smb_dev_name, 000);
	if (smb_dev < 0) {
		perror("open smb dev");
		return 1;
	}


	val = smbioctl_readB(slave);
	printf("recvb is %u(%#02x)\n", val, val);


	close(smb_dev);
	return 0;
}

