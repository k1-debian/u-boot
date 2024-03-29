#ifndef __DDR3_M15T1G1664A_H__
#define __DDR3_M15T1G1664A_H__


#ifndef CONFIG_DDR3_M15T1G1664A_MEM_FREQ
#define CONFIG_DDR3_M15T1G1664A_MEM_FREQ CONFIG_SYS_MEM_FREQ
#endif

#if (CONFIG_DDR_SEL_PLL == MPLL)
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_MPLL_FREQ
#else
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_APLL_FREQ
#endif

#define CONFIG_DDR_DATA_RATE (CONFIG_DDR3_M15T1G1664A_MEM_FREQ * 2)

#if((CONFIG_SYS_PLL_FREQ % CONFIG_DDR3_M15T1G1664A_MEM_FREQ) ||\
    (CONFIG_SYS_PLL_FREQ / CONFIG_DDR3_M15T1G1664A_MEM_FREQ < 0) ||\
    (CONFIG_SYS_PLL_FREQ / CONFIG_DDR3_M15T1G1664A_MEM_FREQ > 15))
#error DDR memoryclock division ratio should be an integer between 1 and 16, check CONFIG_SYS_MPLL_FREQ and CONFIG_DDR3_M15T1G1664A_MEM_FREQ
#endif

/* 
 * CL  WL    MIN      MAX     UNIT   频率范围
 * 6   5     2.5      3.3      ns    303M <= MEM_FREQ <= 400M
 * 7   6     1.875   <2.5      ns    400M <  MEM_FREQ <= 533M
 * 8   6     1.875   <2.5      ns    400M <  MEM_FREQ <= 533M
 * 9   7     1.5     <1.875    ns    533M <  MEM_FREQ <= 667M
 * 10  7     1.5     <1.875    ns    533M <  MEM_FREQ <= 667M
 * 11  8     1.25    <1.5      ns    667M <  MEM_FREQ <= 800M
 * 13  9     1.07    <1.25     ns    800M <  MEM_FREQ <= 933M
 */
#if ((CONFIG_DDR3_M15T1G1664A_MEM_FREQ >= 303000000) && (CONFIG_DDR3_M15T1G1664A_MEM_FREQ <= 400000000))
#define CONFIG_DDR_CL    6
#define CONFIG_DDR_CWL   5
#elif((CONFIG_DDR3_M15T1G1664A_MEM_FREQ > 400000000) && (CONFIG_DDR3_M15T1G1664A_MEM_FREQ <= 533000000))
#define CONFIG_DDR_CL    8  // or CONFIG_DDR_CL 7
#define CONFIG_DDR_CWL   6
#elif((CONFIG_DDR3_M15T1G1664A_MEM_FREQ > 533000000) && (CONFIG_DDR3_M15T1G1664A_MEM_FREQ <= 667000000))
#define CONFIG_DDR_CL    10 // or CONFIG_DDR_CL 9
#define CONFIG_DDR_CWL    7
#elif((CONFIG_DDR3_M15T1G1664A_MEM_FREQ > 667000000) && (CONFIG_DDR3_M15T1G1664A_MEM_FREQ <= 800000000))
#define CONFIG_DDR_CL    11
#define CONFIG_DDR_CWL    8
#elif((CONFIG_DDR3_M15T1G1664A_MEM_FREQ > 800000000) && (CONFIG_DDR3_M15T1G1664A_MEM_FREQ <= 933000000))
#define CONFIG_DDR_CL    13
#define CONFIG_DDR_CWL    9
#elif((CONFIG_DDR3_M15T1G1664A_MEM_FREQ > 933000000) && (CONFIG_DDR3_M15T1G1664A_MEM_FREQ <= 1066000000))
#define CONFIG_DDR_CL    13
#define CONFIG_DDR_CWL    9
#else
#define CONFIG_DDR_CL   -1
#define CONFIG_DDR_CWL  -1
#endif

#if(-1 == CONFIG_DDR_CL)
#error CONFIG_DDR3_M15T1G1664A_MEM_FREQ don't support, check data_rate range
#endif

static inline void DDR3_M15T1G1664A_init(void *data)
{
    struct ddr_chip_info *c = (struct ddr_chip_info *)data;


    c->DDR_ROW           = 13,
    c->DDR_ROW1          = 13,
    c->DDR_COL           = 10,
    c->DDR_COL1          = 10,
    c->DDR_BANK8         = 1,
    c->DDR_BL            = 8,
    c->DDR_CL            = CONFIG_DDR_CL,
    c->DDR_CWL           = CONFIG_DDR_CWL,

    c->DDR_RL            = DDR__tck(c->DDR_CL),
    c->DDR_WL            = DDR__tck(c->DDR_CWL),

    c->DDR_tRAS          = DDR__ns(34);
    c->DDR_tRTP          = DDR_SELECT_MAX__tCK_ps(4, 7500);
    c->DDR_tRP           = DDR__ns(14);
    c->DDR_tRCD          = DDR__ns(14);
    c->DDR_tRC           = c->DDR_tRAS + c->DDR_tRP;
    c->DDR_tRRD          = DDR_SELECT_MAX__tCK_ps(4, 6000);
    c->DDR_tWR           = DDR__ns(15);
    c->DDR_tWTR          = DDR_SELECT_MAX__tCK_ps(4, 7500);
    c->DDR_tCCD          = DDR__tck(4);
    c->DDR_tFAW          = DDR__ns(35);

    c->DDR_tRFC          = DDR__ns(110);
    c->DDR_tREFI         = DDR__ns(3900);

    c->DDR_tCKE          = DDR_SELECT_MAX__tCK_ps(3, 5000);
    c->DDR_tCKESR        = c->DDR_tCKE + DDR__tck(1);
    c->DDR_tCKSRE        = DDR_SELECT_MAX__tCK_ps(5, 10000);
    c->DDR_tXP           = DDR_SELECT_MAX__tCK_ps(3, 6000);
    c->DDR_tMRD          = DDR__tck(4);
    c->DDR_tXSDLL        = DDR__tck(512);
    c->DDR_tMOD          = DDR_SELECT_MAX__tCK_ps(12, 15 * 1000);
    c->DDR_tXPDLL        = DDR_SELECT_MAX__tCK_ps(10, 24 * 1000);
}

#define DDR3_M15T1G1664A {                    \
    .name    = "M15T1G1664A",                    \
    .id      = DDR_CHIP_ID(VENDOR_WINBOND, TYPE_DDR3, MEM_128M),    \
    .type    = DDR3,                        \
    .freq    = CONFIG_DDR3_M15T1G1664A_MEM_FREQ,            \
    .size    = 128,                        \
    .init    = DDR3_M15T1G1664A_init,                \
}


#endif
