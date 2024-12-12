#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <ddr/ddr_common.h>


#define EFUSE_BASE	0xB3540000
#define EFUSE_CTRL	EFUSE_BASE + 0x0
#define EFUSE_CFG	EFUSE_BASE + 0x4
#define EFUSE_STATE	EFUSE_BASE + 0x8
#define EFUSE_DATA	EFUSE_BASE + 0xC

#define EFUSE_CTRL_ADDR_POS	(21)
#define EFUSE_CTRL_LEN_POS	(16)

#define EFUSE_CTRL_RDEN         (1 << 0)
#define EFUSE_STAT_RDDONE       (1 << 0)

#define EFUSE_CFG_RD_ADJ_POS	(20)
#define EFUSE_CFG_RD_STROBE_POS	(16)

#define SOCINFO_ADDR	        0x2B

#define REG32(addr) *(volatile unsigned int *)(addr)

static unsigned int read_socid()
{
	unsigned int val, data;

	REG32(EFUSE_CFG) = 0x2 << EFUSE_CFG_RD_ADJ_POS | 0x5 << EFUSE_CFG_RD_STROBE_POS;

	REG32(EFUSE_CTRL) = 0;
	REG32(EFUSE_CTRL) = SOCINFO_ADDR << EFUSE_CTRL_ADDR_POS | 2 << EFUSE_CTRL_LEN_POS | EFUSE_CTRL_RDEN;
	while(!(REG32(EFUSE_STATE) & EFUSE_STAT_RDDONE));

	data = REG32(EFUSE_DATA);
	val = data & 0xFFFF;

	return val;
}

unsigned int check_socid()
{
	unsigned int vendor = 0;
	unsigned int type = 0;
	unsigned int capacity = 0;
	unsigned int socid  = 0;
	unsigned int ddrid  = 0;

	socid = read_socid();
	if (socid == 0) {
		printf("invalid soc id %x%x\n", socid);
		return -1;
	}
	vendor = socid >> 11 & 0x7;
	type   = socid >> 14 & 0x1;
	capacity = socid >> 8 & 0x7;
	ddrid = DDR_CHIP_ID(vendor, type, capacity);

	return ddrid;
}
