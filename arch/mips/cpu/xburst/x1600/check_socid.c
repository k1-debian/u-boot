#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <ddr/ddr_common.h>


#define EFUSE_BASE	0xB3540000
#define EFUSE_CTRL	EFUSE_BASE + 0x0
#define EFUSE_STATE	EFUSE_BASE + 0x8
#define EFUSE_DATA	EFUSE_BASE + 0xC
#define SOCINFO_ADDR	EFUSE_BASE + 0x2B

#define REG32(addr) *(volatile unsigned int *)(addr)

static unsigned int read_socid()
{
	unsigned int val, data;

	val = SOCINFO_ADDR << 21 | 2 << 16 | 1;
	REG32(EFUSE_CTRL) = val;
	while(!(REG32(EFUSE_STATE) & 1));

	data = REG32(EFUSE_DATA);
	val = data & 0xFFFF;

	return val;
}

unsigned int check_socid()
{
	unsigned int vender = 0;
	unsigned int type = 0;
	unsigned int capacity = 0;
	unsigned int socid  = 0;
	unsigned int ddrid  = 0;

	socid = read_socid();
	vender = socid >> 11 & 0x7;
	type   = socid >> 14 & 0x1;
	capacity = socid >> 8 & 0x7;
	ddrid = DDR_CHIP_ID(vender, type, capacity);

	return ddrid;
}
