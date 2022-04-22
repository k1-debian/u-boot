#ifndef __X1600EN_DDR2_COMPATIBLE_PARAM_H
#define	__X1600EN_DDR2_COMPATIBLE_PARAM_H

static inline void X1600EN_DDR2_init(void *data)
{
	struct ddr_chip_info *c = (struct ddr_chip_info *)data;

	c->DDR_ROW     = 13;
	c->DDR_ROW1    = 13;
	c->DDR_COL     = 10;
	c->DDR_COL1    = 10;

	c->DDR_BANK8   = 0;
	c->DDR_CL      = 7;
	c->DDR_AL      = 0;

	c->DDR_tRAS    = DDR__ns(45);
	c->DDR_tRTP    = DDR__ns(8);
	c->DDR_tRP 	   = DDR__ps(13250);
	c->DDR_tRCD    = DDR__ps(13125);
	c->DDR_tRC     = DDR__ps(58125);
	c->DDR_tRRD    = DDR__ns(10);
	c->DDR_tWR     = DDR__ns(15);
	c->DDR_tWTR    = DDR__ps(7500);
	c->DDR_tRFC    = DDR__ns(105);
	c->DDR_tXP     = DDR__tck(3);
	c->DDR_tMRD    = DDR__tck(2);

	c->DDR_BL      = 8;
	c->DDR_RL      = DDR__tck(c->DDR_AL + c->DDR_CL);
	c->DDR_WL      = DDR__tck(c->DDR_RL - 1);
	c->DDR_tCCD    = DDR__tck(2);
	c->DDR_tFAW    = DDR__ns(45);
	c->DDR_tCKE    = DDR__tck(3) ;
	c->DDR_tCKESR  = DDR__tck(3);
	c->DDR_tXARD   = DDR__tck(3);
	c->DDR_tXARDS  = DDR__tck(10 - c->DDR_AL);

	c->DDR_tXSNR   = (c->DDR_tRFC + DDR__ns(10));
	c->DDR_tXSRD   = DDR__tck(200);
	c->DDR_tREFI   = DDR__ns(7800);

	c->DDR_CLK_DIV = 1;
}

#ifndef CONFIG_X1600EN_DDR2_MEM_FREQ
#define CONFIG_X1600EN_DDR2_MEM_FREQ CONFIG_SYS_MEM_FREQ
#endif

#define X1600EN_DDR2 {					\
	.name 	= "X1600EN-DDR2",					\
	.id	= DDR_CHIP_ID(0, TYPE_DDR2, MEM_64M),	\
	.type	= DDR2,						\
	.freq	= CONFIG_X1600EN_DDR2_MEM_FREQ,			\
	.size	= 64,						\
	.init	= X1600EN_DDR2_init,				\
}

#endif /* __X1600EN_DDR2_COMPATIBLE_PARAM_H */
