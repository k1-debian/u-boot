#ifndef __DDR2_M14D5121632A_H__
#define __DDR2_M14D5121632A_H__



/*
 * CL:5, CWL:5  300M ~ 330M
 * CL:6, CWL:5	300M ~ 400M
 * CL:7, CWL:6
 * CL:8, CWL:6	400M ~ 533M
 * CL:9, CWL:7
 * CL:10, CWL:7 533M ~ 666M
 * CL:11, CWL:8
 * CL:13, CWL:9 800M ~ 933M
 * CL:14, CWL:10
 *
 * */

#define CONFIG_DDR_CL	7
#define CONFIG_DDR_CWL	6


#if defined(CONFIG_DDR_DLL_RESET_EN) || defined(CONFIG_DDR_DLL_OFF)  || \
        defined(DDR2_CHIP_DRIVER_OUT_STRENGHT)
#define CONFIG_DDR2_M14D5121632A_KGD_CONFIG            0x1
#define CONFIG_DDR2_M14D5121632A_KGD_MR0_PD            0x0
#define CONFIG_DDR2_M14D5121632A_KGD_MR1_OCD           0x0
#define CONFIG_DDR2_M14D5121632A_KGD_MR2_DCC_EN        0x0
#endif
#ifdef CONFIG_DDR_DLL_RESET_EN
#define CONFIG_DDR2_M14D5121632A_KGD_MR0_DLL_RST       0x1
#else
#define CONFIG_DDR2_M14D5121632A_KGD_MR0_DLL_RST       0x0
#endif
#ifdef  CONFIG_DDR_DLL_OFF
#define CONFIG_DDR2_M14D5121632A_KGD_MR1_DLL_EN        0x1
#else
#define CONFIG_DDR2_M14D5121632A_KGD_MR1_DLL_EN        0x0
#endif
#ifdef DDR2_CHIP_DRIVER_OUT_STRENGTH
#define CONFIG_DDR2_M14D5121632A_KGD_MR1_DIC           DDR2_CHIP_DRIVER_OUT_STRENGTH
#else
#define CONFIG_DDR2_M14D5121632A_KGD_MR1_DIC           0x1
#endif
#ifdef CONFIG_DDR_CHIP_ODT
#define CONFIG_DDR2_M14D5121632A_KGD_MR1_RTT_NOM       \
        ((CONFIG_DDR_CHIP_ODT_VAL_RTT_NOM_6 << 1) | CONFIG_DDR_CHIP_ODT_VAL_RTT_NOM_2)
#else
#define CONFIG_DDR2_M14D5121632A_KGD_MR1_RTT_NOM       0x1
#endif

#if !defined(CONFIG_DDR2_M14D5121632A_KGD_CONFIG) && \
        defined(CONFIG_DDR2_KGD_CONFIG)
        #define CONFIG_DDR2_M14D5121632A_KGD_CONFIG            CONFIG_DDR2_KGD_CONFIG
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_KGD_MR0_DLL_RST) && \
        defined(CONFIG_DDR2_KGD_MR0_DLL_RST)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR0_DLL_RST       CONFIG_DDR2_KGD_MR0_DLL_RST
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_KGD_MR0_PD) && \
        defined(CONFIG_DDR2_KGD_MR0_PD)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR0_PD            CONFIG_DDR2_KGD_MR0_PD
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_KGD_MR1_DLL_EN) && \
        defined(CONFIG_DDR2_KGD_MR1_DLL_EN)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR1_DLL_EN        CONFIG_DDR2_KGD_MR1_DLL_EN
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_KGD_MR1_DIC) && \
        defined(CONFIG_DDR2_KGD_MR1_DIC)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR1_DIC           CONFIG_DDR2_KGD_MR1_DIC
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_KGD_MR1_RTT_NOM) && \
        defined(CONFIG_DDR2_KGD_MR1_RTT_NOM)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR1_RTT_NOM       CONFIG_DDR2_KGD_MR1_RTT_NOM
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_KGD_MR1_OCD) && \
        defined(CONFIG_DDR2_KGD_MR1_OCD)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR1_OCD           CONFIG_DDR2_KGD_MR1_OCD
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_KGD_MR2_DCC_EN) && \
        defined(CONFIG_DDR2_KGD_MR2_DCC_EN)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR2_DCC_EN        CONFIG_DDR2_KGD_MR2_DCC_EN
#endif

#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DRVODT_CONFIG) && \
        defined(CONFIG_PHY_DRVODT_CONFIG)
        #define CONFIG_DDR2_M14D5121632A_PHY_DRVODT_CONFIG     CONFIG_PHY_DRVODT_CONFIG
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_PU_DRV_CMD) && \
        defined(CONFIG_PHY_PU_DRV_CMD)
        #define CONFIG_DDR2_M14D5121632A_PHY_PU_DRV_CMD        CONFIG_PHY_PU_DRV_CMD
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_PD_DRV_CMD) && \
        defined(CONFIG_PHY_PD_DRV_CMD)
        #define CONFIG_DDR2_M14D5121632A_PHY_PD_DRV_CMD        CONFIG_PHY_PD_DRV_CMD
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_PU_DRV_CK) && \
        defined(CONFIG_PHY_PU_DRV_CK)
        #define CONFIG_DDR2_M14D5121632A_PHY_PU_DRV_CK         CONFIG_PHY_PU_DRV_CK
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_PD_DRV_CK) && \
        defined(CONFIG_PHY_PD_DRV_CK)
        #define CONFIG_DDR2_M14D5121632A_PHY_PD_DRV_CK         CONFIG_PHY_PD_DRV_CK
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_PU_DRV_DQ7_0) && \
        defined(CONFIG_PHY_PU_DRV_DQ7_0)
        #define CONFIG_DDR2_M14D5121632A_PHY_PU_DRV_DQ7_0      CONFIG_PHY_PU_DRV_DQ7_0
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_PD_DRV_DQ7_0) && \
        defined(CONFIG_PHY_PD_DRV_DQ7_0)
        #define CONFIG_DDR2_M14D5121632A_PHY_PD_DRV_DQ7_0      CONFIG_PHY_PD_DRV_DQ7_0
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_PU_DRV_DQ15_8) && \
        defined(CONFIG_PHY_PU_DRV_DQ15_8)
        #define CONFIG_DDR2_M14D5121632A_PHY_PU_DRV_DQ15_8     CONFIG_PHY_PU_DRV_DQ15_8
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_PD_DRV_DQ15_8) && \
        defined(CONFIG_PHY_PD_DRV_DQ15_8)
        #define CONFIG_DDR2_M14D5121632A_PHY_PD_DRV_DQ15_8     CONFIG_PHY_PD_DRV_DQ15_8
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_PU_ODT_DQ7_0) && \
        defined(CONFIG_PHY_PU_ODT_DQ7_0)
        #define CONFIG_DDR2_M14D5121632A_PHY_PU_ODT_DQ7_0      CONFIG_PHY_PU_ODT_DQ7_0
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_PD_ODT_DQ7_0) && \
        defined(CONFIG_PHY_PD_ODT_DQ7_0)
        #define CONFIG_DDR2_M14D5121632A_PHY_PD_ODT_DQ7_0      CONFIG_PHY_PD_ODT_DQ7_0
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_PU_ODT_DQ15_8) && \
        defined(CONFIG_PHY_PU_ODT_DQ15_8)
        #define CONFIG_DDR2_M14D5121632A_PHY_PU_ODT_DQ15_8     CONFIG_PHY_PU_ODT_DQ15_8
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_PD_ODT_DQ15_8) && \
        defined(CONFIG_PHY_PD_ODT_DQ15_8)
        #define CONFIG_DDR2_M14D5121632A_PHY_PD_ODT_DQ15_8     CONFIG_PHY_PD_ODT_DQ15_8
#endif

#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_CONFIG) && \
        defined(CONFIG_PHY_DESKEW_CONFIG)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_CONFIG     CONFIG_PHY_DESKEW_CONFIG
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_CMD) && \
        defined(CONFIG_PHY_DESKEW_CMD)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_CMD        CONFIG_PHY_DESKEW_CMD
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DM0) && \
        defined(CONFIG_PHY_DESKEW_RX_DM0)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DM0     CONFIG_PHY_DESKEW_RX_DM0
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DM0) && \
        defined(CONFIG_PHY_DESKEW_TX_DM0)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DM0     CONFIG_PHY_DESKEW_TX_DM0
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DQ7_0) && \
        defined(CONFIG_PHY_DESKEW_RX_DQ7_0)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DQ7_0   CONFIG_PHY_DESKEW_RX_DQ7_0
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DQ7_0) && \
        defined(CONFIG_PHY_DESKEW_TX_DQ7_0)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DQ7_0   CONFIG_PHY_DESKEW_TX_DQ7_0
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DQS0) && \
        defined(CONFIG_PHY_DESKEW_RX_DQS0)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DQS0    CONFIG_PHY_DESKEW_RX_DQS0
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DQS0) && \
        defined(CONFIG_PHY_DESKEW_TX_DQS0)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DQS0    CONFIG_PHY_DESKEW_TX_DQS0
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DM1) && \
        defined(CONFIG_PHY_DESKEW_RX_DM1)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DM1     CONFIG_PHY_DESKEW_RX_DM1
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DM1) && \
        defined(CONFIG_PHY_DESKEW_TX_DM1)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DM1     CONFIG_PHY_DESKEW_TX_DM1
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DQ15_8) && \
        defined(CONFIG_PHY_DESKEW_RX_DQ15_8)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DQ15_8  CONFIG_PHY_DESKEW_RX_DQ15_8
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DQ15_8) && \
        defined(CONFIG_PHY_DESKEW_TX_DQ15_8)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DQ15_8  CONFIG_PHY_DESKEW_TX_DQ15_8
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DQS1) && \
        defined(CONFIG_PHY_DESKEW_RX_DQS1)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DQS1    CONFIG_PHY_DESKEW_RX_DQS1
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DQS1) && \
        defined(CONFIG_PHY_DESKEW_TX_DQS1)
        #define CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DQS1    CONFIG_PHY_DESKEW_TX_DQS1
#endif

#if defined(CONFIG_DDR_DLL_RESET_EN) || defined(CONFIG_DDR_DLL_OFF)  || \
        defined(DDR2_CHIP_DRIVER_OUT_STRENGHT)
        #define CONFIG_DDR2_M14D5121632A_KGD_CONFIG            0x1

#if defined(CONFIG_DDR_DLL_RESET_EN)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR0_DLL_RST       0x1
#elif !defined(CONFIG_DDR2_M14D5121632A_KGD_MR0_DLL_RST)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR0_DLL_RST       0x0
#endif
#if defined(CONFIG_DDR_DLL_OFF)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR1_DLL_EN        0x1
#elif !defined(CONFIG_DDR2_M14D5121632A_KGD_MR1_DLL_EN)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR1_DLL_EN        0x0
#endif
#if defined(DDR2_CHIP_DRIVER_OUT_STRENGTH)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR1_DIC           DDR2_CHIP_DRIVER_OUT_STRENGTH
#elif !defined(CONFIG_DDR2_M14D5121632A_KGD_MR1_DIC)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR1_DIC           0x1
#endif
#if defined(CONFIG_DDR_CHIP_ODT)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR1_RTT_NOM       \
                ((CONFIG_DDR_CHIP_ODT_VAL_RTT_NOM_6 << 1) | CONFIG_DDR_CHIP_ODT_VAL_RTT_NOM_2)
#elif !defined(CONFIG_DDR2_M14D5121632A_KGD_MR1_RTT_NOM)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR1_RTT_NOM       0x1
#endif

#if !defined(CONFIG_DDR2_M14D5121632A_KGD_MR0_PD)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR0_PD            0x0
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_KGD_MR1_OCD)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR1_OCD           0x0
#endif
#if !defined(CONFIG_DDR2_M14D5121632A_KGD_MR2_DCC_EN)
        #define CONFIG_DDR2_M14D5121632A_KGD_MR2_DCC_EN        0x0
#endif

#endif


static inline void DDR3_M14D5121632A_init(void *data)
{
	struct ddr_chip_info *c = (struct ddr_chip_info *)data;


	c->DDR_ROW  		= 13,
	c->DDR_ROW1 		= 13,
	c->DDR_COL  		= 10,
	c->DDR_COL1 		= 10,
	c->DDR_BANK8 		= 0,
	c->DDR_BL	   	= 8,
	c->DDR_CL	   	= CONFIG_DDR_CL,
	c->DDR_CWL	   	= CONFIG_DDR_CWL,

	c->DDR_RL	   	= DDR__tck(7),
	c->DDR_WL	   	= DDR__tck(6),

	c->DDR_tRAS  		= DDR__ns(45);
	c->DDR_tRTP  		= DDR__ps(7500);
	c->DDR_tRP   		= DDR__ns(15);
	c->DDR_tRCD  		= DDR__ns(15);
	c->DDR_tRC   		= DDR__ps(58125);
	c->DDR_tRRD  		= DDR__ns(10);
	c->DDR_tWR   		= DDR__ns(15);
	c->DDR_tWTR  		= DDR__tck(6) + DDR__tck(2) + DDR__ns(8);
	c->DDR_tCCD  		= DDR__tck(2);
	c->DDR_tFAW  		= DDR__ns(60);

	c->DDR_tRFC  		= DDR__ns(130);
	c->DDR_tREFI 		= DDR__ns(7800);

	c->DDR_tCKE  		= DDR__tck(5);
	c->DDR_tCKESR 		= DDR__tck(0);
	c->DDR_tXP  		= DDR__tck(5);

	c->DDR_tWDLAT       = c->DDR_WL - DDR__tck(1);
	c->DDR_tRTW         = (c->DDR_RL - c->DDR_WL + DDR__tck(5));
	c->DDR_tRDLAT       = (c->DDR_RL - DDR__tck(3));
	c->DDR_tXSNR        = (c->DDR_tRFC + DDR__ns(10));


#ifdef CONFIG_DDR2_M14D5121632A_KGD_CONFIG
        struct ddr2_mr_config *mr_cfg      = &c->kgd_config.mr_config;
        c->kgd_config.use_kgd_config       = CONFIG_DDR2_M14D5121632A_KGD_CONFIG     ;
        mr_cfg->kgd_mr0_dll_rst            = CONFIG_DDR2_M14D5121632A_KGD_MR0_DLL_RST;
        mr_cfg->kgd_mr0_pd                 = CONFIG_DDR2_M14D5121632A_KGD_MR0_PD     ;
        mr_cfg->kgd_mr1_dll_en             = CONFIG_DDR2_M14D5121632A_KGD_MR1_DLL_EN ;
        mr_cfg->kgd_mr1_dic                = CONFIG_DDR2_M14D5121632A_KGD_MR1_DIC    ;
        mr_cfg->kgd_mr1_rtt_nom            = CONFIG_DDR2_M14D5121632A_KGD_MR1_RTT_NOM;
        mr_cfg->kgd_mr1_ocd                = CONFIG_DDR2_M14D5121632A_KGD_MR1_OCD    ;
        mr_cfg->kgd_mr2_dcc_en             = CONFIG_DDR2_M14D5121632A_KGD_MR2_DCC_EN ;
#endif
#ifdef CONFIG_DDR2_M14D5121632A_PHY_DRVODT_CONFIG
        c->phy_drvodt.use_drvodt_config    = CONFIG_DDR2_M14D5121632A_PHY_DRVODT_CONFIG;
        c->phy_drvodt.phy_pu_drv_cmd       = CONFIG_DDR2_M14D5121632A_PHY_PU_DRV_CMD   ;
        c->phy_drvodt.phy_pd_drv_cmd       = CONFIG_DDR2_M14D5121632A_PHY_PD_DRV_CMD   ;
        c->phy_drvodt.phy_pu_drv_ck        = CONFIG_DDR2_M14D5121632A_PHY_PU_DRV_CK    ;
        c->phy_drvodt.phy_pd_drv_ck        = CONFIG_DDR2_M14D5121632A_PHY_PD_DRV_CK    ;
        c->phy_drvodt.phy_pu_drv_dq7_0     = CONFIG_DDR2_M14D5121632A_PHY_PU_DRV_DQ7_0 ;
        c->phy_drvodt.phy_pd_drv_dq7_0     = CONFIG_DDR2_M14D5121632A_PHY_PD_DRV_DQ7_0 ;
        c->phy_drvodt.phy_pu_drv_dq15_8    = CONFIG_DDR2_M14D5121632A_PHY_PU_DRV_DQ15_8;
        c->phy_drvodt.phy_pd_drv_dq15_8    = CONFIG_DDR2_M14D5121632A_PHY_PD_DRV_DQ15_8;
        c->phy_drvodt.phy_pu_odt_dq7_0     = CONFIG_DDR2_M14D5121632A_PHY_PU_ODT_DQ7_0 ;
        c->phy_drvodt.phy_pd_odt_dq7_0     = CONFIG_DDR2_M14D5121632A_PHY_PD_ODT_DQ7_0 ;
        c->phy_drvodt.phy_pu_odt_dq15_8    = CONFIG_DDR2_M14D5121632A_PHY_PU_ODT_DQ15_8;
        c->phy_drvodt.phy_pd_odt_dq15_8    = CONFIG_DDR2_M14D5121632A_PHY_PD_ODT_DQ15_8;
#endif
#ifdef CONFIG_DDR2_M14D5121632A_PHY_DESKEW_CONFIG
        c->phy_deskew.use_deskew_config    = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_CONFIG   ;
        c->phy_deskew.phy_deskew_cmd       = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_CMD      ;
        c->phy_deskew.phy_deskew_rx_dm0    = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DM0   ;
        c->phy_deskew.phy_deskew_tx_dm0    = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DM0   ;
        c->phy_deskew.phy_deskew_rx_dq7_0  = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DQ7_0 ;
        c->phy_deskew.phy_deskew_tx_dq7_0  = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DQ7_0 ;
        c->phy_deskew.phy_deskew_rx_dqs0   = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DQS0  ;
        c->phy_deskew.phy_deskew_tx_dqs0   = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DQS0  ;
        c->phy_deskew.phy_deskew_rx_dm1    = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DM1   ;
        c->phy_deskew.phy_deskew_tx_dm1    = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DM1   ;
        c->phy_deskew.phy_deskew_rx_dq15_8 = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DQ15_8;
        c->phy_deskew.phy_deskew_tx_dq15_8 = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DQ15_8;
        c->phy_deskew.phy_deskew_rx_dqs1   = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_RX_DQS1  ;
        c->phy_deskew.phy_deskew_tx_dqs1   = CONFIG_DDR2_M14D5121632A_PHY_DESKEW_TX_DQS1  ;
#endif
}


#ifndef CONFIG_DDR2_M14D5121632A_MEM_FREQ
#define CONFIG_DDR2_M14D5121632A_MEM_FREQ CONFIG_SYS_MEM_FREQ
#endif

#define DDR2_M14D5121632A {					\
	.name 	= "M14D5121632A",					\
	.id	= DDR_CHIP_ID(VENDOR_ESMT, TYPE_DDR2, MEM_64M),	\
	.type	= DDR2,						\
	.freq	= CONFIG_DDR2_M14D5121632A_MEM_FREQ,			\
	.size	= 64,						\
	.init	= DDR3_M14D5121632A_init,				\
}


#endif
