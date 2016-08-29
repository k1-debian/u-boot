#include <config.h>
#include <common.h>
#include <asm/io.h>

#define REG32(addr) *(volatile unsigned int *)(addr)

#define EFUSE_CTRL		0xb3540000
#define EFUSE_CFG		0xb3540004
#define EFUSE_STATE		0xb3540008
#define EFUSE_DATA0		0xb354000C
static enum socid {
	X1000 = 0xff00,
	X1000E = 0xff01,
	X1500 = 0xff02,
};
static void read_socid(unsigned int *id)
{
	unsigned int val;

	/* clear read done staus */
	REG32(EFUSE_STATE) = 0;
	val = 0x3c << 21 | 1 << 16 | 1;
	REG32(EFUSE_CTRL) = val;
	/* wait read done status */
	while(!(REG32(EFUSE_STATE) & 1))
		;
	*id = REG32(EFUSE_DATA0) & 0xffff;
	/* clear read done staus */
	REG32(EFUSE_STATE) = 0;
}
int check_socid()
{
	unsigned int socid;

	read_socid(&socid);
	switch(socid) {
	case X1000:
		break;
	case X1000E:
		break;
	case X1500:
		break;
	default:
		socid = -1;
	}
	return 0;
}
