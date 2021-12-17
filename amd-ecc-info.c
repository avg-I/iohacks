#include <sys/types.h>
#include <sys/fcntl.h>

#include <err.h>
#include <inttypes.h>
#include <libutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/pciio.h>
#include <sys/queue.h>

static uint32_t
read_config(int fd, struct pcisel *sel, long reg, int width)
{
	struct pci_io pi;

	pi.pi_sel = *sel;
	pi.pi_reg = reg;
	pi.pi_width = width;

	if (ioctl(fd, PCIOCREAD, &pi) < 0)
		err(1, "ioctl(PCIOCREAD)");

	return (pi.pi_data);
}

static uint64_t
get_pci(int bus, int dev, int func, int reg)
{
	static int fd = -1;
	struct pcisel sel;

	if (fd == -1) {
		fd = open("/dev/pci", O_RDWR);
	}
	if (fd == -1) {
		perror("/dev/pci");
		return (-1);
	}

	sel.pc_domain = 0;
	sel.pc_bus = bus;
	sel.pc_dev = dev;
	sel.pc_func = func;
	return (read_config(fd, &sel, reg, 4));
}

#ifdef notyet
static void
write_config(int fd, struct pcisel *sel, long reg, uint32_t val, int width)
{
	struct pci_io pi;

	pi.pi_sel = *sel;
	pi.pi_reg = reg;
	pi.pi_data = val;
	pi.pi_width = width;

	if (ioctl(fd, PCIOCWRITE, &pi) < 0)
		err(1, "ioctl(PCIOCREAD)");
}

static void
set_pci(int bus, int dev, int func, int reg, uint32_t val)
{
	static int fd = -1;
	struct pcisel sel;

	if (fd == -1) {
		fd = open("/dev/pci", O_RDWR);
	}
	if (fd == -1) {
		perror("/dev/pci");
	}

	sel.pc_domain = 0;
	sel.pc_bus = bus;
	sel.pc_dev = dev;
	sel.pc_func = func;
	write_config(fd, &sel, reg, val, 4);
}
#endif

uint32_t
get_nb_cap(void)
{
	return (get_pci(0, 24, 3, 0xe8));
}

void
print_nb_cap(void)
{
	uint32_t cfg = get_nb_cap();

	printf("NB ECC: %scapable\n", (cfg & (1 << 3)) != 0 ? "" : "not ");
	printf("NB Chipkill: %scapable\n", (cfg & (1 << 4)) != 0 ? "" : "not ");
}

uint32_t
get_dram_scrub_rate(void)
{
	return (get_pci(0, 24, 3, 0x58) & 0xff);
}

void
print_dram_scrub_rate(void)
{
	char buf[16];
	uint32_t rate = get_dram_scrub_rate();

	printf("DRAM scrub: ");
	if (rate == 0) {
		printf("disabled");
	} else if (rate > 0x16) {
		printf("<invalid value>");
	} else {
		uint32_t period_ns = 40 << (rate - 1);
		uint32_t speed = (1000000000 / period_ns) * 64;

		humanize_number(buf, sizeof(buf), speed, "B/s", HN_AUTOSCALE, 0);
		printf("%s", buf);
		printf(" (%u seconds to scrub 1 GB)", 1024 * 1024 * 1024 / speed);
		if (rate <= 4)
			printf(" [scrub runs when there are no other memory accesses]");
	}
	printf("\n");
}

uint32_t
get_nb_mca_cfg(void)
{
	return (get_pci(0, 24, 3, 0x44));
}

uint32_t
get_ext_nb_mca_cfg(void)
{
	return (get_pci(0, 24, 3, 0x180));
}

void
print_nb_mca_cfg(void)
{
	uint32_t cfg = get_nb_mca_cfg();
	int enabled = (cfg & (1 << 22)) != 0;

	printf("DRAM ECC: %s\n", enabled ? "enabled" : "disabled");
	if (!enabled)
		return;
	printf("Chipkill: %scapable\n", (cfg & (1 << 23)) != 0 ? "" : "not ");
	printf("Sync-flood on UC error: %s\n", (cfg & (1 << 2)) != 0 ? "yes" : "no");

	cfg = get_ext_nb_mca_cfg();
	printf("ECC symbol size: %s\n", (cfg & (1 << 25)) != 0 ? "x8" : "x4");

	print_dram_scrub_rate();
}

uint32_t
get_dct_cfg(void)
{
	return (get_pci(0, 24, 2, 0x110));
}

uint32_t
get_dct_x_cfg(int index, int low_hi)
{
#if 0
	/* Bulldozer, etc */
	uint32_t sel = get_pci(0, 24, 1, 0x10c);
	index &= 1;
	sel &= ~(uint32_t)1;
	sel |= index;
	set_pci(0, 24, 1, 0x10c, sel);
#endif
	return (get_pci(0, 24, 2, 0x90 +
	    (index ? 0x100 : 0) + (low_hi ? 4 : 0)));
}

uint32_t
get_cs_cfg(int dct, int cs)
{
	return (get_pci(0, 24, 2, 0x40 + (dct ? 0x100 : 0) + cs * 4));
}

void
print_dct_cfg(void)
{
	uint32_t cfg = get_dct_cfg();
	int ganged;

	if ((cfg & (1 << 10)) != 0)
		printf("Memory cleared since reboot\n");
	ganged = (cfg & (1 << 4)) != 0;
	printf("Ganged mode enabled: %s\n",
	    ganged ? "yes" : "no");
	printf("Data interleave (128-bit ECC word is formed from two "
	    "consecutive 64-bit lines): %s\n",
	    (cfg & (1 << 5)) != 0 ? "yes" : "no");
	printf("Controller interleave enabled: %s\n",
	    (cfg & (1 << 2)) != 0 ? "yes" : "no");

	for (int i = 0; i <= 1; i++) {
		int disabled;

		cfg = get_dct_x_cfg(i, 1);
		disabled = (cfg & (1 << 14)) != 0;
		printf("DCT%d: %sabled\n", i,
		    disabled ? "dis" : "en");
		if (disabled)
			continue;

		cfg = get_dct_x_cfg(i, 0);
		printf("DCT%d DIMM(s) ECC capable: %s\n", i,
		    (cfg & (1 << 19)) != 0 ? "yes" : "no");
		printf("DCT%d UDIMM(s): %s\n", i,
		    (cfg & (1 << 16)) != 0 ? "yes" : "no");
		printf("DCT%d x4 DIMM mask: 0x%x\n", i,
		    (cfg & (0xf << 12)) >> 12);
		printf("DCT%d width: %d bit\n", i,
		    (cfg & (1 << 11)) != 0 ? 128 : 64);

		cfg = get_dct_x_cfg(i, 1);
		printf("DCT%d ODT: %sabled\n", i,
		    (cfg & (1 << 23)) != 0 ? "dis" : "en");
		printf("DCT%d bank swizzle: %sabled\n", i,
		    (cfg & (1 << 22)) != 0 ? "en" : "dis");
		printf("DCT%d 2T mode: %sabled\n", i,
		    (cfg & (1 << 20)) != 0 ? "en" : "dis");
		printf("DCT%d power-down: %sabled\n", i,
		    (cfg & (1 << 15)) != 0 ? "en" : "dis");
		printf("DCT%d power-down mode: %s\n", i,
		    (cfg & (1 << 16)) != 0 ? "CS" : "channel");

		int ddr3 = (cfg & (1 << 8)) != 0;
		printf("DCT%d memory type: DDR%d\n", i,
		    ddr3 ? 3 : 2);

		int freq = 0;
		switch (cfg & 0x7) {
			case 0:
				freq = ddr3 ? 0 : 200;
				break;
			case 1:
				freq = ddr3 ? 0 : 266;
				break;
			case 2:
				freq = 333;
				break;
			case 3:
				freq = 400;
				break;
			case 4:
				freq = 533;
				break;
			case 5:
				freq = ddr3 ? 667 : 0;
				break;
			case 6:
				freq = ddr3 ? 800 : 0;
				break;
		}
		if (freq != 0)
			printf("DCT%d frequency: %d MHZ\n", i, freq);

		printf("CS present:");
		for (int c = 0; c < 8; c++) {
			cfg = get_cs_cfg(i, c);
			if ((cfg & 1) != 0)
				printf(" %d", c);
		}
		printf("\n");
	}
}

int main()
{
	print_nb_cap();
	print_nb_mca_cfg();
	print_dct_cfg();
	return (0);
}
