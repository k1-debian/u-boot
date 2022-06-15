#ifndef __X1600EN_DDR2_COMPATIBLE_PARAM_H
#define	__X1600EN_DDR2_COMPATIBLE_PARAM_H

/*
 * CL:3,50M ~ 200M
 * CL:4,200M ~ 266M
 * CL:5,266M ~ 400M
 * CL:6,
 * CL:7,400M ~ 533M
 *
 * */
#if ((CONFIG_SYS_MEM_FREQ > 50000000) && (CONFIG_SYS_MEM_FREQ <= 266000000))
#define CONFIG_DDR_CL	4
#elif((CONFIG_SYS_MEM_FREQ > 266000000) && (CONFIG_SYS_MEM_FREQ <= 333000000))
#define CONFIG_DDR_CL	5
#elif((CONFIG_SYS_MEM_FREQ > 333000000) && (CONFIG_SYS_MEM_FREQ <= 400000000))
#define CONFIG_DDR_CL	6
#elif((CONFIG_SYS_MEM_FREQ > 400000000) && (CONFIG_SYS_MEM_FREQ <= 533000000))
#define CONFIG_DDR_CL	7
#else
#define CONFIG_DDR_CL	0
#endif

#define CONFIG_DDR_AL	0

static inline void X1600EN_DDR2_init(void *data)
{
	struct ddr_chip_info *c = (struct ddr_chip_info *)data;
	unsigned int RL = CONFIG_DDR_CL + CONFIG_DDR_AL;

	c->DDR_ROW     = 13;
	c->DDR_ROW1    = 13;
	c->DDR_COL     = 10;
	c->DDR_COL1    = 10;

	c->DDR_BANK8   = 0;
	c->DDR_CL      = CONFIG_DDR_CL;
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
	c->DDR_RL      = DDR__tck(RL);
	c->DDR_WL      = DDR__tck(RL - 1);
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
