#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/base.h>
#include <asm/arch/efuse.h>

DECLARE_GLOBAL_DATA_PTR;
#define REG32(addr) *(volatile unsigned int *)(addr)

#define DDR_TIMING4	    0x07230f31
#define DDR_64M_CFG	    0x0a468a40
#define DDR_64M_MMAP0	    0x000020fc
#define DDR_64M_MMAP1	    0x00002400
#define DDR_64M_REMMAP0	    0x03020d0c
#define DDR_64M_REMMAP2	    0x0b0a0908
#define DDR_64M_REMMAP3	    0x0f0e0100

#define DDRP_MR2_DS_BIT             5
#define DDRP_MR2_DS_MASK           (0x7 << DDRP_MR2_DS_BIT)
#define DDRC_TIMING4_tXP_BIT	    4
#define DDRC_TIMING4_tXP_MASK	    (0x7 << DDRC_TIMING4_tXP_BIT)
#define DDRC_TIMING4_tXP_6	    (6 << DDRC_TIMING4_tXP_BIT)
#define DDRC_CFG_ODT_MASK	    (1U << 16)

enum ddr_change_param {
	REMMAP0,
	REMMAP1,
	REMMAP2,
	REMMAP3,
};


static struct soc_desc {
	unsigned int id;
	const char *chip;
};

static const struct soc_desc desc[] = {
	{SOCID_X1000,       "X1000" },
	{SOCID_X1000E,      "X1000E"},
	{SOCID_X1500,       "X1500"},
	{SOCID_X1500L,      "X1500L" },
	{SOCID_X1000_NEW,   "X1000_NEW"},
	{SOCID_X1000E_NEW,  "X1000E_NEW"},
	{SOCID_X1500_NEW,   "X1500_NEW"},
	{SOCID_X1501,       "X1501"},
};


static void read_efuse_segment(unsigned int addr, unsigned int length, unsigned int *buf)
{
	unsigned int val;

	/* clear read done staus */
	REG32(EFUSE_BASE + EFUSE_STATE) = 0;
	val = (addr - EFUSE_ROM_BASE) << EFUSE_CTRL_ADDR |
	      length << EFUSE_CTRL_LEN | EFUSE_CTRL_RDEN;
	REG32(EFUSE_BASE + EFUSE_CTRL) = val;
	/* wait read done status */
	while(!(REG32(EFUSE_BASE + EFUSE_STATE) & EFUSE_STA_RD_DONE))
		;
	if(addr == EFUSE_SOCID_ADDR && length == 1) {
		buf[0] = REG32(EFUSE_BASE + EFUSE_DATA(0)) & EFUSE_SOCID_MASK;
	} else if(addr == EFUSE_CHIPID_ADDR) {
		buf[0] = REG32(EFUSE_BASE + EFUSE_DATA(1));
		buf[1] = REG32(EFUSE_BASE + EFUSE_DATA(3));
	} else if (addr == EFUSE_TRIM1_ADDR) {
		buf[0] = REG32(EFUSE_BASE + EFUSE_DATA(0));
	}
	/* clear read done staus */
	REG32(EFUSE_BASE + EFUSE_STATE) = 0;
}

void read_socid(unsigned int *id)
{
	read_efuse_segment(EFUSE_SOCID_ADDR, 1, id);
}

static inline int check_chipid(unsigned int *data)
{
	unsigned int lotid_l, lotid_h;
	unsigned int waferid;
#define LOTID_LOW 0x07
#define LOTID_LOW_MASK 0x1F
#define LOTID_HIGH 0x0E90E02F
#define LOTID_HIGH_MASK 0x3FFFFFFF
#define WAFERID_BIT_OFF 11
#define WAFERID_MASK 0x1f

	lotid_l = data[0] & LOTID_LOW_MASK;
	lotid_h = data[1] & LOTID_HIGH_MASK;
	waferid = (data[0] >> WAFERID_BIT_OFF) & WAFERID_MASK;

	/* printf("lotid_l = %x, lotid_h = %x\n", lotid_l, lotid_h); */
	/* printf("waferid = %x\n", waferid); */
	if(lotid_l == LOTID_LOW && lotid_h == LOTID_HIGH)
		if(waferid >=16 && waferid <= 25)
			return 0;
	return -1;

}
static int read_and_check_chipid()
{
	unsigned int val, data[2];

	read_efuse_segment(EFUSE_CHIPID_ADDR, 15, data);
	return check_chipid(data);
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

#ifdef CONFIG_DDR_EFUSE_OVERRIDES
static unsigned int ddr_apply_efuse_overrides()
{
	unsigned int trim1;

	/* EFUSE_CTRL_LEN encodes the byte count minus one. */
	read_efuse_segment(EFUSE_TRIM1_ADDR, 3, &trim1);

	/* Override flags apply to every SLT version. */
	if (trim1 & EFUSE_TRIM1_DDR_DS0_TXP_6) {
		gd->arch.gi->ddr_change_param.ddr_mr2 &= ~DDRP_MR2_DS_MASK;
		gd->arch.gi->ddr_change_param.ddr_timing4 &= ~DDRC_TIMING4_tXP_MASK;
		gd->arch.gi->ddr_change_param.ddr_timing4 |= DDRC_TIMING4_tXP_6;
	}
	if (trim1 & EFUSE_TRIM1_DDR_AUTOSR_DISABLE)
		gd->arch.gi->ddr_change_param.ddr_autosr = 0;
	if (trim1 & EFUSE_TRIM1_DDR_ODT_DISABLE)
		gd->arch.gi->ddr_change_param.ddr_cfg &= ~DDRC_CFG_ODT_MASK;

    return trim1;
}
#endif

int check_socid(unsigned int *ddr_id, char *chip_name)
{
	int i = 0;
	unsigned int socid;
    unsigned int trim1 = 0;

	read_socid(&socid);
	if (ddr_id)
		*ddr_id = socid;

	for (i = 0; i < ARRAY_SIZE(desc); i++) {
		if (desc[i].id == socid) {

			if (chip_name)
				strcpy(chip_name, desc[i].chip);

			if (SOCID_X1000_NEW == socid || SOCID_X1500_NEW == socid ||
			    SOCID_X1500L == socid || SOCID_X1501 == socid ||
			    (SOCID_X1500 == socid && !read_and_check_chipid())) {
				gd->arch.gi->ddr_change_param.ddr_autosr = 1;
			} else if (SOCID_X1000E == socid || SOCID_X1000E_NEW == socid) {
				ddr_change_64M();
				gd->arch.gi->ddr_change_param.ddr_autosr = 1;
			} else if (SOCID_X1000 == socid) {
				gd->arch.gi->ddr_change_param.ddr_timing4 = DDR_TIMING4;
			}

#ifdef CONFIG_DDR_EFUSE_OVERRIDES
			if (SOCID_X1000_NEW == socid || SOCID_X1500_NEW == socid)
            trim1 = ddr_apply_efuse_overrides();
#endif

			return (int)trim1;
		}
	}

	return -1;
}
