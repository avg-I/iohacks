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
smbioctl_Bwrite(int slave, int addr, char* buf, size_t size)
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
	const int slave = 0x69 << 1;

	char data[7];
	int i;

	/* See W40S11 and W48S111 specs */
	data[0] = 0xCF; data[0] = 0xFF;
	data[1] = 0xF3; data[1] = 0xFF;
	data[2] = 0x40; data[2] = 0xC0;

	/* 0x8: 1 - soft-control, 0 - pin-control */
	/* 0x2: spread spectrum enable/disable */
	/* 0x70: 100Mhz */
	/* 0x60: 133Mhz */
	/* 0x50: 112Mhz */
	data[3] = 0x68; /* 100Mhz, soft-controlled */
	data[4] = 0x45; /* cpu0 + cpu1 */
	data[5] = 0xEF; /* default */
	data[6] = 0x23; /* default */
	smb_dev = open(smb_dev_name, 000);
	if (smb_dev < 0) {
		perror("open smb dev");
		return 1;
	}
	smbioctl_Bwrite(slave, 0, data, sizeof(data));
	for (i = 0; i < sizeof(data); ++i) {
		printf("wrote %u(%#02x) = %u(%#02x)\n", i, i, (unsigned char)data[i], (unsigned char)data[i]);
	}

	close(smb_dev);
	return 0;
}

