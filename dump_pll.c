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

static void
smbioctl_Bread(int slave, int addr, char* buf, size_t size)
{
        struct smbcmd cmd;

        cmd.slave = slave;
        cmd.cmd = addr; /*XXX zero?*/
	cmd.count = size;
        cmd.data.byte_ptr = buf;
        if (ioctl(smb_dev, SMB_BREAD, &cmd) == -1) {
                perror("ioctl_read");
                exit(-1);
        }
}

static void
smbioctl_Bwrite(int slave, int addr, const char* buf, size_t size)
{
        struct smbcmd cmd;

        cmd.slave = slave;
        cmd.cmd = addr; /*XXX zero?*/
	cmd.count = size;
        cmd.data.byte_ptr = buf;
        if (ioctl(smb_dev, SMB_BWRITE, &cmd) == -1) {
                perror("ioctl_write");
                exit(-1);
        }
}

int main(int argc, char** argv)
{
	const int slave = 0xd2;

	char data[0x20];
	int i;

	smb_dev = open(smb_dev_name, 000);
	if (smb_dev < 0) {
		perror("open smb dev");
		return 1;
	}
	smbioctl_Bread(slave, 0, data, sizeof(data));
	for (i = 0; i < sizeof(data)/sizeof(data[0]); ++i) {
		printf("read %u(%#02x) = %u(%#02x)\n", i, i, (unsigned char)data[i], (unsigned char)data[i]);
	}

	close(smb_dev);
	return 0;
}

