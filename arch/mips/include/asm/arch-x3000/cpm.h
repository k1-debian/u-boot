/*
 * X3000 CPM definitions.
 */
#ifndef __X3000_CPM_H__
#define __X3000_CPM_H__

#include <asm/arch/base.h>

#define CPM_CPCCR		0x00
#define CPM_CPCCR1		0x04
#define CPM_CPCSR		0x08
#define CPM_CPAPCR		0x10
#define CPM_CPMPCR		0x14
#define CPM_CPEPCR		0x18
#define CPM_DDRCDR		0x30
#define CPM_MSC0CDR		0x34
#define CPM_MSC1CDR		0x38
#define CPM_MSC2CDR		0x3c
#define CPM_SFCCDR		0x40
#define CPM_SADCCDR		0x78
#define CPM_SADC1CDR		0x7c
#define CPM_USBRDT		0x200
#define CPM_USBPCR		0x204
#define CPM_USBPCR1		0x208
#define CPM_USB1RDT		0x210
#define CPM_USB1PCR		0x214
#define CPM_USB1PCR1		0x218
#define CPM_CLKGR0		0x100
#define CPM_CLKGR1		0x104
#define CPM_CLKGR2		0x108
#define CPM_SR			0x120
#define CPM_BC			0x128
#define CPM_BC1			0x12c
#define CPM_CLKGR0_SET		0x130
#define CPM_CLKGR1_SET		0x134
#define CPM_CLKGR2_SET		0x138
#define CPM_SR_SET		0x150
#define CPM_BC_SET		0x158
#define CPM_BC1_SET		0x15c
#define CPM_CLKGR0_CLR		0x160
#define CPM_CLKGR1_CLR		0x164
#define CPM_CLKGR2_CLR		0x168
#define CPM_SR_CLR		0x180
#define CPM_BC_CLR		0x188
#define CPM_BC1_CLR		0x18c
#define CPM_DRCG		0x254	/* Obsolete on X3000; do not use for new DDR init. */

#define cpm_inl(off)		readl(CPM_BASE + (off))
#define cpm_outl(val, off)	writel(val, CPM_BASE + (off))
#define cpm_clear_bit(val, off)	do { cpm_outl((cpm_inl(off) & ~(1 << (val))), off); } while (0)
#define cpm_set_bit(val, off)	do { cpm_outl((cpm_inl(off) | (1 << (val))), off); } while (0)
#define cpm_test_bit(val, off)	(cpm_inl(off) & (1 << (val)))
#define cpm_writel(val, off)	writel(val, CPM_BASE + (off))
#define cpm_readl(off)		readl(CPM_BASE + (off))

/* CPCCR bits */
#define CPCCR_SEL_SRC_MASK	(3 << 30)
#define CPCCR_SEL_SRC_STOP	(0 << 30)
#define CPCCR_SEL_SRC_EXCLK	(1 << 30)
#define CPCCR_SEL_SRC_APLL	(2 << 30)
#define CPCCR_SEL_LEPPLL_MASK	(3 << 28)
#define CPCCR_SEL_LEPPLL_STOP	(0 << 28)
#define CPCCR_SEL_LEPPLL_SCLKA	(1 << 28)
#define CPCCR_SEL_LEPPLL_MPLL	(2 << 28)
#define CPCCR_SEL_CPLL_MASK	(3 << 24)
#define CPCCR_SEL_CPLL_STOP	(0 << 24)
#define CPCCR_SEL_CPLL_SCLKA	(1 << 24)
#define CPCCR_SEL_CPLL_MPLL	(2 << 24)
#define CPCCR_GATE_SCLKA	(1 << 19)
#define CPCCR_CE_LEP		(1 << 18)
#define CPCCR_CE_CPU		(1 << 16)
#define CPCCR_LEPDIV_SHIFT	8
#define CPCCR_L2DIV_SHIFT	4
#define CPCCR_CDIV_SHIFT	0
#define CPCCR_DIV_MASK		0xf

/* CPCCR1 bits */
#define CPCCR1_SEL_H2PLL_MASK	(3 << 28)
#define CPCCR1_SEL_H2PLL_STOP	(0 << 28)
#define CPCCR1_SEL_H2PLL_SCLKA	(1 << 28)
#define CPCCR1_SEL_H2PLL_MPLL	(2 << 28)
#define CPCCR1_SEL_H1PLL_MASK	(3 << 26)
#define CPCCR1_SEL_H1PLL_STOP	(0 << 26)
#define CPCCR1_SEL_H1PLL_SCLKA	(1 << 26)
#define CPCCR1_SEL_H1PLL_MPLL	(2 << 26)
#define CPCCR1_SEL_H0PLL_MASK	(3 << 24)
#define CPCCR1_SEL_H0PLL_STOP	(0 << 24)
#define CPCCR1_SEL_H0PLL_SCLKA	(1 << 24)
#define CPCCR1_SEL_H0PLL_MPLL	(2 << 24)
#define CPCCR1_CE_AHB2		(1 << 18)
#define CPCCR1_CE_AHB1		(1 << 17)
#define CPCCR1_CE_AHB0		(1 << 16)
#define CPCCR1_PDIV_SHIFT	12
#define CPCCR1_H2DIV_SHIFT	8
#define CPCCR1_H1DIV_SHIFT	4
#define CPCCR1_H0DIV_SHIFT	0

/* CPCSR bits */
#define CPCSR_DDR_MUX		(1 << 31)
#define CPCSR_AHB2_MUX		(1 << 30)
#define CPCSR_AHB1_MUX		(1 << 29)
#define CPCSR_AHB0_MUX		(1 << 28)
#define CPCSR_SRC_MUX		(1 << 27)
#define CPCSR_LEP_MUX		(1 << 26)
#define CPCSR_CPU_MUX		(1 << 24)
#define CPCSR_H2DIV_BUSY	(1 << 6)
#define CPCSR_H1DIV_BUSY	(1 << 5)
#define CPCSR_H0DIV_BUSY	(1 << 4)
#define CPCSR_LEPDIV_BUSY	(1 << 2)
#define CPCSR_CDIV_BUSY		(1 << 0)

/* CLKGR0 bits */
#define CPM_CLKGR_DDR		(1 << 8)
#define CPM_CLKGR_SFC		(1 << 20)
#define CPM_CLKGR_MSC0		(1 << 21)
#define CPM_CLKGR_MSC1		(1 << 22)
#define CPM_CLKGR_MSC2		(1 << 23)
#define CPM_CLKGR_EFUSE		(1 << 16)

/* CLKGR1 bits */
#define CPM_CLKGR_UART0		(1 << 0)
#define CPM_CLKGR_UART1		(1 << 2)
#define CPM_CLKGR_UART2		(1 << 3)
#define CPM_CLKGR_UART3		(1 << 4)
#define CPM_CLKGR_UART4		(1 << 5)
#define CPM_CLKGR_UART5		(1 << 6)
#define CPM_CLKGR_UART6		(1 << 1)
#define CPM_CLKGR_UART7		(1 << 7)

/* SR bits */
#define CPM_DDRP_RESET		(1 << 31)
#define CPM_DDRP_RELEASE_RESET	(1 << 31)
#define CPM_DDRC_RESET		(1 << 30)
#define CPM_DDRC_RELEASE_RESET	(1 << 30)


/* USB reset-detect timer IDDIG controls. */
#define USBRDT_IDDIG_EN		(1 << 24)
#define USBRDT_IDDIG_REG	(1 << 23)

/* BC bits */
#define CPM_BC_DDR0_STP		(1 << 10)
#define CPM_BC_DDR0_ACK		(1 << 11)
#define CPM_BC_DDR0_CH0_STP	(1 << 12)
#define CPM_BC_DDR0_CH0_ACK	(1 << 13)
#define CPM_BC_DDR0_CH1_STP	(1 << 14)
#define CPM_BC_DDR0_CH1_ACK	(1 << 15)
#define CPM_BC_DDR0_CH2_STP	(1 << 16)
#define CPM_BC_DDR0_CH2_ACK	(1 << 17)
#define CPM_BC_DDR0_CH3_STP	(1 << 18)
#define CPM_BC_DDR0_CH3_ACK	(1 << 19)
#define CPM_BC_DDR0_CH4_STP	(1 << 20)
#define CPM_BC_DDR0_CH4_ACK	(1 << 21)
#define CPM_BC_DDR0_CH5_STP	(1 << 22)
#define CPM_BC_DDR0_CH5_ACK	(1 << 23)
#define CPM_BC_DDR0_CH6_STP	(1 << 24)
#define CPM_BC_DDR0_CH6_ACK	(1 << 25)
#define CPM_BC_DDR0_STP_MASK	(CPM_BC_DDR0_STP | CPM_BC_DDR0_CH0_STP | \
				 CPM_BC_DDR0_CH1_STP | CPM_BC_DDR0_CH2_STP | \
				 CPM_BC_DDR0_CH3_STP | CPM_BC_DDR0_CH4_STP | \
				 CPM_BC_DDR0_CH5_STP | CPM_BC_DDR0_CH6_STP)
#define CPM_BC_DDR0_ACK_MASK	(CPM_BC_DDR0_ACK | CPM_BC_DDR0_CH0_ACK | \
				 CPM_BC_DDR0_CH1_ACK | CPM_BC_DDR0_CH2_ACK | \
				 CPM_BC_DDR0_CH3_ACK | CPM_BC_DDR0_CH4_ACK | \
				 CPM_BC_DDR0_CH5_ACK | CPM_BC_DDR0_CH6_ACK)

/* DDRCDR bits */
#define DDRCDR_DCS_BIT		30
#define DDRCDR_DCS_MASK		(3 << DDRCDR_DCS_BIT)
#define DDRCDR_DCS_STOP		(0 << DDRCDR_DCS_BIT)
#define DDRCDR_DCS_SCLK_A	(1 << DDRCDR_DCS_BIT)
#define DDRCDR_DCS_MPLL		(2 << DDRCDR_DCS_BIT)
#define DDRCDR_CE_DDR		(1 << 29)
#define DDRCDR_DDR_BUSY		(1 << 28)
#define DDRCDR_DDR_STOP		(1 << 27)
#define DDRCDR_DDR_AUTO_STP	(1 << 26)
#define DDRCDR_CHANGE_EN	(1 << 25)
#define DDRCDR_FLAG		(1 << 24)
#define DDRCDR_DDR_PHY_STP_EN	(1 << 23)
#define DDRCDR_DIV_MASK		0xf

#define MSCCDR_EXCK_E		(1 << 21)
#define MSCCDR_TUNING_DIS	(1 << 20)
#define MSCCDR_MPCS		30
#define MSCCDR_MPCS_MASK	(3 << MSCCDR_MPCS)
#define MSCCDR_MPCS_APLL	(0 << MSCCDR_MPCS)
#define MSCCDR_MPCS_MPLL	(1 << MSCCDR_MPCS)
#define MSCCDR_MPCS_EXCLK	(2 << MSCCDR_MPCS)

#define RECOVERY_SIGNATURE	0x1a1a
#define FASTBOOT_SIGNATURE	0x0666

#endif /* __X3000_CPM_H__ */
