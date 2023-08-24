#ifndef __DDR2_W9751V6NG_CONFIG_H
#define	__DDR2_W9751V6NG_CONFIG_H

/*
 * CL:4,50M ~ 333M
 * CL:5,333M ~ 400M
 * CL:6,
 * CL:7,400M ~ 533M
 *
 * */
#ifndef CONFIG_DDR2_W9751V6NG_MEM_FREQ
#define CONFIG_DDR2_W9751V6NG_MEM_FREQ CONFIG_SYS_MEM_FREQ
#endif

#if (CONFIG_DDR_SEL_PLL == MPLL)
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_MPLL_FREQ
#else
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_APLL_FREQ
#endif

#define CONFIG_DDR_DATA_RATE (CONFIG_DDR2_W9751V6NG_MEM_FREQ * 2)

#if((CONFIG_SYS_PLL_FREQ % CONFIG_DDR2_W9751V6NG_MEM_FREQ) ||\
	(CONFIG_SYS_PLL_FREQ / CONFIG_DDR2_W9751V6NG_MEM_FREQ < 0) ||\
	(CONFIG_SYS_PLL_FREQ / CONFIG_DDR2_W9751V6NG_MEM_FREQ > 15))
#error DDR memoryclock division ratio should be an integer between 1 and 16, check CONFIG_SYS_MPLL_FREQ and CONFIG_DDR2_W9751V6NG_MEM_FREQ;
#endif

#if ((CONFIG_DDR_DATA_RATE > 266000000) &&\
		(CONFIG_DDR_DATA_RATE < 800000000))
#define CONFIG_DDR_CL	5
#elif((CONFIG_DDR_DATA_RATE >= 800000000) &&\
		(CONFIG_DDR_DATA_RATE <= 1066000000))
#define CONFIG_DDR_CL	7
#elif((CONFIG_DDR_DATA_RATE > 1066000000) &&\
		(CONFIG_DDR_DATA_RATE <= 1333000000))
#define CONFIG_DDR_CL	9
#else
#define CONFIG_DDR_CL	-1
#endif

#define CONFIG_DDR_AL	0

#if(-1 == CONFIG_DDR_CL)
#error CONFIG_DDR2_W9751V6NG_MEM_FREQ don't support, check %s\n, check data_rate range
#endif

static inline void DDR2_W9751V6NG_init(void *data)
{
	struct ddr_chip_info *c = (struct ddr_chip_info *)data;
	unsigned int RL = CONFIG_DDR_CL + CONFIG_DDR_AL;

	c->DDR_ROW = 13;
	c->DDR_ROW1 = 13;
	c->DDR_COL = 10;
	c->DDR_COL1 = 10;

	c->DDR_BANK8 = 0;
	c->DDR_CL = CONFIG_DDR_CL;
	c->DDR_AL = CONFIG_DDR_AL;

	c->DDR_tRAS = DDR__ns(45);
	c->DDR_tRTP = DDR_SELECT_MAX__tCK_ps(4,7500);
	c->DDR_tRP = DDR__ns(12);
	c->DDR_tRCD = DDR__ns(12);
	c->DDR_tRC = DDR__ns(57);
	c->DDR_tRRD = DDR__ns(10);
	c->DDR_tWR = DDR__ps(13500);
	c->DDR_tWTR = DDR_SELECT_MAX__tCK_ps(4,7500);
	c->DDR_tRFC = DDR__ns(120);
	c->DDR_tXP = DDR__tck(4);
	c->DDR_tMRD = DDR__tck(2);

	c->DDR_BL = 8;
	c->DDR_RL = DDR__tck(RL);
	c->DDR_WL = DDR__tck(RL - 1);
	c->DDR_tCCD = DDR__tck(2);
	c->DDR_tFAW = DDR__ns(45);
	c->DDR_tCKE = DDR__tck(3);
	c->DDR_tCKESR = DDR__tck(3);
	c->DDR_tXARD = DDR__tck(4);
	c->DDR_tXARDS = DDR__tck(12 - c->DDR_AL);

	c->DDR_tXSNR = (c->DDR_tRFC + DDR__ns(10));
	c->DDR_tXSRD = DDR__tck(200);
	c->DDR_tREFI = DDR__ns(3900);

	c->DDR_CLK_DIV = 1;
}

#define DDR2_W9751V6NG {					\
	.name 	= "W9751V6NG",					\
	.id	= DDR_CHIP_ID(VENDOR_WINBOND, TYPE_DDR2, MEM_64M),	\
	.type	= DDR2,						\
	.freq	= CONFIG_DDR2_W9751V6NG_MEM_FREQ,			\
	.size	= 64,						\
	.init	= DDR2_W9751V6NG_init,				\
}

#endif /* __DDR2_CONFIG_H */
