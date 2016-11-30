#include <config.h>
#include <common.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;
#define REG32(addr) *(volatile unsigned int *)(addr)

#define EFUSE_CTRL	    0xb3540000
#define EFUSE_CFG	    0xb3540004
#define EFUSE_STATE	    0xb3540008
#define EFUSE_DATA0	    0xb354000C

#define DDR_64M_CFG	    0x0a668a40
#define DDR_64M_MMAP0	    0x000020fc
#define DDR_64M_MMAP1	    0x00002400
#define DDR_64M_TIMING4     0x07230f21
#define DDR_64M_REMMAP0	    0x03020d0c
#define DDR_64M_REMMAP2	    0x0b0a0908
#define DDR_64M_REMMAP3	    0x0f0e0100

enum ddr_change_param {
	REMMAP0,
	REMMAP1,
	REMMAP2,
	REMMAP3,
};

static enum socid {
	X1000 = 0xff00,
	X1000E = 0xff01,
	X1500 = 0xff02,
	X1000_NEW = 0xff08,
	X1000E_NEW = 0xff09,
	X1500_NEW = 0xff0a,
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

static void ddr_change_64M()
{
	uint32_t *remmap = gd->arch.gi->ddr_change_param.ddr_remap_array;
	gd->arch.gi->ddr_change_param.ddr_cfg = DDR_64M_CFG;
	gd->arch.gi->ddr_change_param.ddr_mmap0 = DDR_64M_MMAP0;
	gd->arch.gi->ddr_change_param.ddr_mmap1 = DDR_64M_MMAP1;
	/*remmap*/
	remmap[REMMAP0] = DDR_64M_REMMAP0;
	remmap[REMMAP2] = DDR_64M_REMMAP2;
	remmap[REMMAP3] = DDR_64M_REMMAP3;

}

int check_socid()
{
	unsigned int socid;

	read_socid(&socid);
	switch(socid) {
	case X1000_NEW:
		gd->arch.gi->ddr_change_param.ddr_autosr = 1;
		break;
	case X1000E:
	case X1000E_NEW:
		ddr_change_64M();
		gd->arch.gi->ddr_change_param.ddr_autosr = 1;
		break;
	case X1500_NEW:
	case X1500:
	case X1000:
	case 0:
		gd->arch.gi->ddr_change_param.ddr_timing4 = DDR_64M_TIMING4;
		break;
	default:
		socid = -1;
	}

	return socid;
}
