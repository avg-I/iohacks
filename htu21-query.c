
#include <err.h>
#include <errno.h>
#include <sysexits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <dev/iicbus/iic.h>

#define	I2C_DEV			"/dev/iic0"

#define	HTU21_GET_TEMP		0xe3
#define	HTU21_GET_HUM		0xe5
#define	HTU21_GET_TEMP_NH	0xf3
#define	HTU21_GET_HUM_NH	0xf5

#define	HTU21_SLAVE_ADDR	0x40

static int hold_mode = 0;
static int xfer_mode = 0;
static int verbosity = 0;

#if 0
static uint8_t
calc_crc(uint8_t data, uint8_t crc)
{
	static const uint8_t polynomial = 0x8c;
	int i;

	for (i = 0; i < 8; i++) {
		int lsb_neq = (data ^ crc) & 1;

		data >>= 1;
		crc >>= 1;
		if (lsb_neq)
			crc ^= polynomial;
	}
	return (crc);
}

static int
valid_crc(const uint8_t *data, int count, uint8_t expected)
{
	int i;
	uint8_t crc;

	crc = 0;
	for (i = 0; i < count; i++)
	    crc = calc_crc(data[i], crc);
	fprintf(stderr, "crc: %02x, expected: %02x\n", crc, expected);
	return (crc == expected);
}
#else
uint8_t
calc_crc(uint16_t data)
{
	static const uint16_t polynomial = 0x3100;
	int i;

	for (i = 0; i < 16; i++) {
		int msb_neq = data & 0x8000;

		data <<= 1;
		if (msb_neq)
			data ^= polynomial;
	}
	return (data >> 8);
}

static int
valid_crc(const uint8_t *data, uint8_t expected)
{
	uint8_t crc;

	crc = calc_crc(((uint16_t)data[0] << 8) | data[1]);
	//fprintf(stderr, "crc: %02x, expected: %02x\n", crc, expected);
	return (crc == expected);
}
#endif

static int
transfer_raw(int fd, uint8_t cmd, uint8_t *data, int count)
{
	struct iic_msg msgs[2];
	struct iic_rdwr_data xfer;
	int error;

	msgs[0].slave = HTU21_SLAVE_ADDR << 1;
	msgs[0].flags = IIC_M_WR | (hold_mode ? IIC_M_NOSTOP : 0);
	msgs[0].len = 1;
	msgs[0].buf = &cmd;

	msgs[1].slave = HTU21_SLAVE_ADDR << 1;
	msgs[1].flags = IIC_M_RD;
	msgs[1].len = count;
	msgs[1].buf = data;

	if (hold_mode) {
		xfer.msgs = msgs;
		xfer.nmsgs = 2;

		error = ioctl(fd, I2CRDWR, &xfer);
		if (error != 0) {
			fprintf(stderr, "error executing transfer: %d\n",
			    errno);
		}
		return (error);
	}

	xfer.msgs = msgs;
	xfer.nmsgs = 1;

	error = ioctl(fd, I2CRDWR, &xfer);
	if (error != 0) {
		fprintf(stderr, "error executing command transfer: %d\n",
		    errno);
		return (error);
	}

	for (;;) {
		xfer.msgs = &msgs[1];
		xfer.nmsgs = 1;
		error = ioctl(fd, I2CRDWR, &xfer);
		if (error != 0 && errno == 2 /*IIC_ENOACK*/) {
			usleep(40000);
			continue;
		}
		if (error != 0) {
			fprintf(stderr, "error executing data transfer: %d\n",
			    errno);
			break;
		}
		return (0);
	}
	return (error);
}

static int
request_raw(int fd, uint8_t cmd, uint8_t *data, int count)
{
	struct iiccmd req;
	int error;

	req.slave = HTU21_SLAVE_ADDR << 1;
	req.count = 0;
	req.last = 0;
	req.buf = NULL;

	if (!xfer_mode)
		error = ioctl(fd, I2CSTART, &req);
	if (xfer_mode || (error != 0 && errno == ENODEV)) {
		if (!xfer_mode && verbosity > 0) {
			fprintf(stderr, "explicit start not supported\n");
			fprintf(stderr, "fallback to transfer mode\n");
		}
		return (transfer_raw(fd, cmd, data, count));
	}
	if (error != 0)	/* allow one retry */
		error = ioctl(fd, I2CSTART, &req);
	if (error != 0) {
		fprintf(stderr, "error sending start condition: %d\n", errno);
		(void)ioctl(fd, I2CSTOP);
		return (error);
	}

	req.buf = (char *)&cmd;
	req.count = 1;
	req.last = 0;
	error = ioctl(fd, I2CWRITE, &req);
	if (error != 0) {
		fprintf(stderr, "error sending command: %d\n", errno);
		(void) ioctl(fd, I2CSTOP);
		return (error);
	}

	if (!hold_mode) {
		error = ioctl(fd, I2CSTOP);
		if (error != 0) {
			fprintf(stderr, "error sending stop condition\n");
			return (error);
		}
	}

	for (;;) {
		req.slave = (HTU21_SLAVE_ADDR << 1) | 1;
		req.count = 0;
		req.last = 0;
		req.buf = NULL;
		if (hold_mode)
			error = ioctl(fd, I2CRPTSTART, &req);
		else
			error = ioctl(fd, I2CSTART, &req);
		if (error != 0) {
			if (!hold_mode) {
				usleep(40000);
				continue;
			} else {
				fprintf(stderr,
				    "error sending r-start condition: %d\n",
				    errno);
				(void)ioctl(fd, I2CSTOP);
				return (error);
			}
		}

		req.buf = (char *)data;
		req.count = count;
		req.last = 1;
		error = ioctl(fd, I2CREAD, &req);
		if (error != 0)
			fprintf(stderr, "error reading response: %d\n", errno);

		(void)ioctl(fd, I2CSTOP);
		return (error);
	}
}

int main(int argc, char **argv)
{
	const char *dev;
	long value;
	int ch;
	int error;
	int fd;
	int retval;
	uint16_t raw;
	uint8_t data[3];
	uint8_t status;

	while ((ch = getopt(argc, argv, "Hvx")) != -1) {
		switch (ch) {
		case 'H':
			hold_mode = 1;
			break;
		case 'v':
			verbosity++;
			break;
		case 'x':
			xfer_mode = 1;
			break;
		case '?':
		default:
			fprintf(stderr, "unknow option\n");
		}
	}
	argc -= optind;
	argv += optind;

	dev = I2C_DEV;
	if (argc > 0)
		dev = argv[0];

	if (verbosity > 0)
		fprintf(stderr, "using %s\n", dev);

	fd = open(dev, O_RDWR);
	if (fd == -1)
		err(1, "open failed");

	retval = 0;
	error = request_raw(fd, hold_mode ? HTU21_GET_TEMP : HTU21_GET_TEMP_NH,
	    data, sizeof(data));
	if (error != 0) {
		fprintf(stderr, "failed to read temperature data\n");
		retval = 1;
	} else if (!valid_crc(data, data[2])) {
		fprintf(stderr, "CRC check failed for temperature data\n");
		fprintf(stderr, "%02x:%02x:%02x\n", data[0], data[1], data[2]);
		retval = 1;
	} else {
		raw = (((uint16_t)data[0]) << 8) | (data[1] & 0xfc);
		status = data[1] & 0x3;
		if ((status & 1) != 0) {
			fprintf(stderr,
			    "inconsistency: status bit zero is set\n");
		}
		if ((status & 2) != 0) {
			fprintf(stderr,
			    "inconsistency: humidity status bit is set\n");
		}
		if (raw == 0) {
			fprintf(stderr, "read all zeroes: %02x:%02x:%02x\n",
			    data[0], data[1], data[2]);
			retval = 1;
		} else {
			value = ((uint64_t)raw * 17572) >> 16;
			value -= 4685;
			printf("Temperature: %ld.%02lu degC\n",
			    value / 100, value % 100);
		}
	}

	error = request_raw(fd, hold_mode ? HTU21_GET_HUM : HTU21_GET_HUM_NH,
	    data, sizeof(data));
	if (error != 0) {
		fprintf(stderr, "failed to read humidity data\n");
		retval = 1;
	} else if (!valid_crc(data, data[2])) {
		fprintf(stderr, "CRC check failed for humidity data\n");
		fprintf(stderr, "%02x:%02x:%02x\n", data[0], data[1], data[2]);
		retval = 1;
	} else {
		raw = (((uint16_t)data[0]) << 8) | (data[1] & 0xfc);
		status = data[1] & 0x3;
		if ((status & 1) != 0) {
			fprintf(stderr,
			    "inconsistency: status bit zero is set\n");
		}
		if ((status & 2) == 0) {
			fprintf(stderr,
			    "inconsistency: humidity status bit is not set\n");
		}
		if (raw == 0) {
			fprintf(stderr, "read all zeroes: %02x:%02x:%02x\n",
			    data[0], data[1], data[2]);
			retval = 1;
		} else {
			value = ((uint64_t)raw * 12500) >> 16;
			value -= 600;
			printf("Humidity: %ld.%02lu %%\n",
			    value / 100, value % 100);
		}
	}

	return (retval);
}
