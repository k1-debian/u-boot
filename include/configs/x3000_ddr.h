#ifndef __X3000_DDR_H__
#define __X3000_DDR_H__

#define CONFIG_DDR_INNOPHY
#define CONFIG_DDR_HOST_CC

#if !defined(CONFIG_DDR_X3000_DWC_INNOPHY) && \
	!defined(CONFIG_DDR_X3000_DWC_INNOPHY_DDR4)
#error "X3000 DDR currently supports only X3000 DWC INNOPHY DDR paths"
#endif

#define CONFIG_DDR_X3000_DWC_INNOPHY_COMMON

/*
 * DWC DDR controller bring-up target for X3000 chip verification.
 *
 * The controller/PHY init sequence and register values are provided by the
 * X3000 ddr module, so the legacy DDR params creator is not used here.
 */
/*#define CONFIG_DDR_TYPE_DDR3*/
#define CONFIG_DDR_TYPE_DDR4
#define CONFIG_DDR3_W631GU6NG

#define CONFIG_DDR_CS0				1
#define CONFIG_DDR_CS1				0
#define CONFIG_DDR_DW32				0

#define CONFIG_DDR3_KGD_CONFIG			0x1
#define CONFIG_DDR3_KGD_MR0_DLL_RST		0x1
#define CONFIG_DDR3_KGD_MR0_PD			0x1
#define CONFIG_DDR3_KGD_MR1_DLL_EN		0x0
#define CONFIG_DDR3_KGD_MR1_DIC			0x1
#define CONFIG_DDR3_KGD_MR1_RTT_NOM		0x1
#define CONFIG_DDR3_KGD_MR2_RTT_WR		0x0

#define CONFIG_PHY_DRVODT_CONFIG		0x1
#define CONFIG_PHY_PU_DRV_CMD			0xc
#define CONFIG_PHY_PD_DRV_CMD			0xc
#define CONFIG_PHY_PU_DRV_CK			0xc
#define CONFIG_PHY_PD_DRV_CK			0xc
#define CONFIG_PHY_PU_DRV_DQ7_0			0xc
#define CONFIG_PHY_PD_DRV_DQ7_0			0xc
#define CONFIG_PHY_PU_DRV_DQ15_8		0xc
#define CONFIG_PHY_PD_DRV_DQ15_8		0xc
#define CONFIG_PHY_PU_ODT_DQ7_0			0x2
#define CONFIG_PHY_PD_ODT_DQ7_0			0x2
#define CONFIG_PHY_PU_ODT_DQ15_8		0x2
#define CONFIG_PHY_PD_ODT_DQ15_8		0x2

#define CONFIG_DDR_PHY_IMPEDANCE		40
#define CONFIG_DDR_PHY_ODT_IMPEDANCE		120
#define CONFIG_DDR_AUTO_SELF_REFRESH_CNT	257

#define CONFIG_BOOTARGS_MEM_64M			"mem=64M@0x0"
#define CONFIG_BOOTARGS_MEM_128M		"mem=128M@0x0"
#define CONFIG_BOOTARGS_MEM_256M		"mem=256M@0x0"
#define CONFIG_BOOTARGS_MEM_512M		"mem=256M@0x0 mem=256M@0x30000000"

#endif /* __X3000_DDR_H__ */
