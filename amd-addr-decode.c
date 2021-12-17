#include <sys/types.h>
#include <sys/fcntl.h>

#include <err.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/pciio.h>
#include <sys/queue.h>

uint32_t
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

uint64_t
Get_PCI(int bus, int dev, int func, int reg)
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

int
fUnaryXOR(uint32_t v)
{
	int count;

	count = __builtin_popcount(v);
	return (count & 1);
}

typedef struct result {
	int found;
	int node;
	int channel;
	int cs;
} result_t;

result_t TranslateSysAddrToCS(uint64_t SystemAddr)
{
	result_t result;

	int SwapDone, BadDramCs;
	int CSFound, NodeID, CS, F1Offset, F2Offset, F2MaskOffset, Ilog;
	int HiRangeSelected, DramRange;
	int DramEn, DctGangEn, CSEn, DctSelIntLvEn, DctSelHiRngEn;
	int Channel;
	uint32_t IntlvEn, IntlvSel;
	uint32_t DramBaseLow, DramLimitLow;
	uint32_t HoleOffset, HoleEn;
	uint32_t CSBase, CSMask;
	uint32_t InputAddr, Temp;
	uint32_t OnlineSpareCTL;
	uint32_t DctSelBaseAddr, DctSelIntLvAddr;
	uint32_t DctSelHi;
	uint64_t DramBaseLong, DramLimitLong;
	uint64_t DctSelBaseOffsetLong, ChannelOffsetLong, ChannelAddrLong;

	// device is a user supplied value for the PCI device ID of the processor
	// from which CSRs are initially read from (current processor is fastest).
	// CH0SPARE_RANK and CH1SPARE_RANK are user supplied values, determined
	// by BIOS during DIMM sizing.
	const int bus0 = 0;
	const int device = 24;
	const int dev24 = 24;
	const int func1 = 1;
	const int func2 = 2;
	const int func3 = 3;
	const int CH0SPARE_RANK = -1;
	const int CH1SPARE_RANK = -2;

	CSFound = 0;
	for (DramRange = 0; DramRange < 8; DramRange++) {
		F1Offset = 0x40 + (DramRange << 3);
		DramBaseLow = Get_PCI(bus0, device, func1, F1Offset);
		DramEn = (DramBaseLow & 0x00000003) != 0;
		if (!DramEn)
			continue;

		IntlvEn = (DramBaseLow & 0x00000700) >> 8;
		DramBaseLow &= 0xFFFF0000;
		DramBaseLong = (((Get_PCI(bus0, device, func1, F1Offset + 0x100) & 0xFF)<<32) +
				DramBaseLow)<<8;

		DramLimitLow = Get_PCI(bus0, device, func1, F1Offset + 4);
		IntlvSel = (DramLimitLow & 0x00000700) >> 8;
		NodeID = DramLimitLow & 0x00000007;

		DramLimitLow |= 0x0000FFFF;
		DramLimitLong = (((Get_PCI(bus0, device, func1, F1Offset + 0x104) & 0xFF)<<32) +
				DramLimitLow)<<8 | 0xFF;

		if (SystemAddr < DramBaseLong || SystemAddr > DramLimitLong)
			continue;

		if (IntlvEn != 0 && IntlvSel != ((SystemAddr >> 12) & IntlvEn))
			continue;

		if (IntlvEn == 1)
			Ilog = 1;
		else if (IntlvEn == 3)
			Ilog = 2;
		else if (IntlvEn == 7)
			Ilog = 3;
		else
			Ilog = 0;

		Temp = Get_PCI(bus0, dev24 + NodeID, func2, 0x10C);
		int IntLvRgnSwapEn = Temp & 0x1;
		if (IntLvRgnSwapEn) {
			uint32_t IntLvRgnBaseAddr = Temp >> 3 & 0x7F;
			uint32_t IntLvRgnLmtAddr = Temp >> 11 & 0x7F;
			uint32_t IntLvRgnSize = Temp >> 20 & 0x7F;

			if ((SystemAddr >> 34) == 0 &&
			    (((SystemAddr >> 27) >= IntLvRgnBaseAddr &&
			    (SystemAddr >> 27) <= IntLvRgnLmtAddr) ||
			    (SystemAddr >> 27) < IntLvRgnSize)) {
				SystemAddr ^= IntLvRgnBaseAddr << 27;
			}
		}

		Temp = Get_PCI(bus0, dev24 + NodeID, func2, 0x110);
		DctSelHiRngEn = Temp & 1;
		DctSelHi = (Temp >> 1) & 1;
		DctSelIntLvEn = (Temp & 4) != 0;
		DctGangEn = (Temp & 0x10) != 0;
		DctSelIntLvAddr = Temp>>6 & 3;
		DctSelBaseAddr = Temp & 0xFFFFF800;
		DctSelBaseOffsetLong = (Get_PCI(bus0, dev24 + NodeID, func2, 0x114) &
				0xFFFFFC00)<<16;

		//Determine if High range is selected
		if (DctSelHiRngEn && DctGangEn == 0 &&
		    (SystemAddr>>27) >= (DctSelBaseAddr>>11))
			HiRangeSelected = 1;
		else
			HiRangeSelected = 0;

		//Determine Channel
		if (DctGangEn)
			Channel = 0;
		else if (HiRangeSelected)
			Channel = DctSelHi;
		else if (DctSelIntLvEn && DctSelIntLvAddr == 0)
			Channel = (SystemAddr >> 6) & 1;
		else if (DctSelIntLvEn && ((DctSelIntLvAddr>>1) & 1) != 0) {
			Temp = fUnaryXOR((SystemAddr>>16)&0x1F); //function returns odd parity
			//1= number of set bits in argument is odd.
			//0= number of set bits in argument is even.
			if (DctSelIntLvAddr & 1)
				Channel = (SystemAddr>>9 & 1)^Temp;
			else
				Channel = (SystemAddr>>6 & 1)^Temp;
		}
		else if (DctSelIntLvEn && (IntlvEn & 4) != 0)
			Channel = (SystemAddr >> 15) & 1;
		else if (DctSelIntLvEn && (IntlvEn & 2) != 0)
			Channel = (SystemAddr >> 14) & 1;
		else if (DctSelIntLvEn && (IntlvEn & 1) != 0)
			Channel = (SystemAddr >> 13) & 1;
		else if (DctSelIntLvEn)
			Channel = (SystemAddr >> 12) & 1;
		else if (DctSelHiRngEn)
			Channel = ~DctSelHi & 1;
		else
			Channel = 0;

		HoleEn = Get_PCI(bus0, dev24 + NodeID, func1, 0xF0);
		HoleOffset = HoleEn & 0x0000FF80;
		HoleEn = HoleEn & 0x00000003;

		//Determine Base address Offset to use
		if (HiRangeSelected) {
			if ((DctSelBaseAddr & 0xFFFF0000ul) == 0 &&
			    (HoleEn & 1) != 0 && SystemAddr >= 0x100000000ul)
				ChannelOffsetLong = HoleOffset << 16;
			else
				ChannelOffsetLong = DctSelBaseOffsetLong;
		} else {
			if ((HoleEn & 1) != 0 && SystemAddr >= 0x100000000ul)
				ChannelOffsetLong = HoleOffset << 16;
			else
				ChannelOffsetLong = DramBaseLong & 0xFFFFF8000000ul;
		}

		//Remove hoisting offset and normalize to DRAM bus addresses
		ChannelAddrLong = (SystemAddr & 0x0000FFFFFFFFFFC0ul) -
			(ChannelOffsetLong & 0x0000FFFFFF800000ul);

		//Remove Node ID (in case of processor interleaving)
		Temp = ChannelAddrLong & 0xFC0;
		ChannelAddrLong = ((ChannelAddrLong >> Ilog) & 0xFFFFFFFFF000ul) | Temp;

		//Remove Channel interleave and hash
		if (DctSelIntLvEn && HiRangeSelected == 0 && DctGangEn == 0) {
			if ((DctSelIntLvAddr & 1) == 0) {
				ChannelAddrLong = (ChannelAddrLong>>1) & 0xFFFFFFFFFFFFFFC0ul;
			} else if (DctSelIntLvAddr == 1) {
				Temp = ChannelAddrLong & 0xFC0;
				ChannelAddrLong = ((ChannelAddrLong & 0xFFFFFFFFFFFFE000ul) >> 1)| Temp;
			} else {
				Temp = ChannelAddrLong & 0x1C0;
				ChannelAddrLong = ((ChannelAddrLong & 0xFFFFFFFFFFFFFC00ul) >> 1)| Temp;
			}
		}

		InputAddr = ChannelAddrLong >> 8;
		for (CS = 0; CS < 8; CS++) {
			F2Offset = 0x40 + Channel * 0x100 + (CS << 2);
			F2MaskOffset = 0x60 + Channel * 0x100 + ((CS >> 1) << 2);
			CSBase = Get_PCI(bus0, dev24 + NodeID, func2, F2Offset);
			CSEn = (CSBase & 0x00000001) != 0;
			CSBase = CSBase & 0x1FF83FE0;
			CSMask = Get_PCI(bus0, dev24 + NodeID, func2, F2MaskOffset);
			CSMask = (CSMask | 0x0007C01F) & 0x1FFFFFFF;
			if (CSEn && ((InputAddr & ~CSMask) == (CSBase & ~CSMask))) {
				OnlineSpareCTL = Get_PCI(bus0, dev24 + NodeID, func3, 0xB0);
				if (Channel > 0) {
					SwapDone = (OnlineSpareCTL >> 3) & 0x00000001;
					BadDramCs = (OnlineSpareCTL >> 8) & 0x00000007;
					if (SwapDone && CS == BadDramCs)
						CS = CH1SPARE_RANK;
				} else {
					SwapDone = (OnlineSpareCTL >> 1) & 0x00000001;
					BadDramCs = (OnlineSpareCTL >> 4) & 0x00000007;
					if (SwapDone && CS == BadDramCs)
						CS = CH0SPARE_RANK;
				}
				CSFound = 1;
				break;
			}
		}

		if (CSFound)
			break;
	} // for each DramRange

	result.found = CSFound;
	result.node = NodeID;
	result.channel = Channel;
	result.cs = CS;

	return (result);
}

int main(int argc, char **argv)
{
	result_t result;
	uint64_t addr;

	if (argc < 2) {
		fprintf(stderr, "Usage %s <address>\n", argv[0]);
		return (1);
	}
	addr = strtoull(argv[1], NULL, 0);
	result = TranslateSysAddrToCS(addr);
	if (!result.found) {
		printf("could not map address 0x%lx\n", addr);
		return (1);
	}

	printf("node: %d\n", result.node);
	printf("chan: %d\n", result.channel);
	printf("cs: %d\n", result.cs);

	return (0);
}

