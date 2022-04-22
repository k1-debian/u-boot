#ifndef __X1660L_LVDDR_COMPATIBLE_PARAM_H
#define	__X1660L_LVDDR_COMPATIBLE_PARAM_H

static inline void X1660L_LVDDR_init(void *data)
{
	struct ddr_chip_info *c = (struct ddr_chip_info *)data;

	c->DDR_ROW     = 12;
	c->DDR_ROW1    = 12;
	c->DDR_COL     = 8;
	c->DDR_COL1    = 8;
	c->DDR_BANK8   = 0;
	c->DDR_CL      = 3;

	c->DDR_tRAS    = DDR__ns(40);
	c->DDR_tRP     = DDR__ns(15);
	c->DDR_tRCD    = DDR__ns(15);
	c->DDR_tRC     = (c->DDR_tRAS + c->DDR_tRP);
	c->DDR_tRRD    = DDR__ns(10);
	c->DDR_tWR     = DDR__ns(15);
	c->DDR_tWTR    = DDR__tck(2);
	c->DDR_tRFC    = DDR__ns(72);
	c->DDR_tXP     = DDR__ns(25);
	c->DDR_tMRD    = DDR__tck(2);

	c->DDR_BL      = 4;
	c->DDR_RL      = DDR__tck(c->DDR_CL);
	c->DDR_WL      = DDR__tck(1);
	c->DDR_tCKE    = DDR__tck(2);
	c->DDR_tXSR    = DDR__tck(200);
	c->DDR_tREFI   = DDR__ns(3900);

#ifdef CONFIG_LVDDR_INNOPHY
	c->DDR_tRTP    = DDR__ns(8);
	c->DDR_tCCD    = DDR__tck(2);
	c->DDR_tRTW    = (((c->DDR_BL > 4) ? 6 : 4) + 1);
	c->DDR_tFAW    = DDR__ns(45);
	c->DDR_tXARD   = DDR__tck(2);
	c->DDR_tXARDS  = DDR__tck(7);
	c->DDR_tXSNR   = (c->DDR_tRFC + DDR__ns(10));
	c->DDR_tXSRD   = DDR__tck(200);
	c->DDR_tCKESR  = DDR__tck(3);
	c->DDR_tCKSRE  = DDR__ns(10000);

	c->DDR_CLK_DIV = 1;
#endif
}

#ifndef CONFIG_X1660L_LVDDR_MEM_FREQ
#define CONFIG_X1660L_LVDDR_MEM_FREQ CONFIG_SYS_MEM_FREQ
#endif

#define X1660L_LVDDR {					\
	.name 	= "X1660L-LVDDR",					\
	.id	= DDR_CHIP_ID(0, TYPE_DDR2, MEM_8M),	\
	.type	= DDR2,						\
	.freq	= CONFIG_X1660L_LVDDR_MEM_FREQ,			\
	.size	= 8,						\
	.init	= X1660L_LVDDR_init,				\
}

#endif /* __MDDR_CONFIG_H */


