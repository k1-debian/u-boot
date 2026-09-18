#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>
#include <generated/pll_reg_values.h>

#define X3000_PLL_TIMEOUT	0x10000

static void x3000_clk_dump(const char *stage)
{
	serial_debug("x3000 clk dump[%s]\n", stage);
	serial_debug("  CPAPCR=%x CPMPCR=%x CPEPCR=%x\n",
		     cpm_inl(CPM_CPAPCR), cpm_inl(CPM_CPMPCR),
		     cpm_inl(CPM_CPEPCR));
	serial_debug("  CPCCR=%x CPCCR1=%x CPCSR=%x\n",
		     cpm_inl(CPM_CPCCR), cpm_inl(CPM_CPCCR1),
		     cpm_inl(CPM_CPCSR));
	serial_debug("  CLKGR0=%x CLKGR1=%x DDRCDR=%x MSC0CDR=%x\n",
		     cpm_inl(CPM_CLKGR0), cpm_inl(CPM_CLKGR1),
		     cpm_inl(CPM_DDRCDR), cpm_inl(CPM_MSC0CDR));
}

static int wait_cpcsr_clear(unsigned int busy_mask, unsigned int stable_mask,
			    const char *stage)
{
	unsigned int timeout = X3000_PLL_TIMEOUT;
	unsigned int val;

	do {
		val = cpm_inl(CPM_CPCSR);
		if (!(val & busy_mask) && ((val & stable_mask) == stable_mask))
			return 0;
	} while (--timeout);

	serial_debug("x3000 %s timeout: CPCSR=%x busy=%x stable=%x\n",
		     stage, val, busy_mask, stable_mask);
	return -1;
}

static void pll_set(unsigned int reg)
{
	unsigned int val;
	unsigned int timeout;

	val = cpm_inl(reg);
	val &= ~(1 << 0);
	cpm_outl(val, reg);

	switch (reg) {
	case CPM_CPAPCR:
		val = (APLL_EN_VALUE << 0) | (APLL_M_VALUE << 20) |
		      (APLL_N_VALUE << 14) | (APLL_OD1_VALUE << 11) |
		      (APLL_OD0_VALUE << 8);
		break;
	case CPM_CPMPCR:
		val = (MPLL_EN_VALUE << 0) | (MPLL_M_VALUE << 20) |
		      (MPLL_N_VALUE << 14) | (MPLL_OD1_VALUE << 11) |
		      (MPLL_OD0_VALUE << 8);
		break;
	case CPM_CPEPCR:
		val = (EPLL_EN_VALUE << 0) | (EPLL_M_VALUE << 20) |
		      (EPLL_N_VALUE << 14) | (EPLL_OD1_VALUE << 11) |
		      (EPLL_OD0_VALUE << 8);
		break;
	default:
		return;
	}

	cpm_outl(val, reg);
	timeout = X3000_PLL_TIMEOUT;
	while ((!(cpm_inl(reg) & (1 << 3))) && --timeout)
		;
	if (!timeout) {
		serial_debug("pll reg[0x%x] val[0x%x] setting timeout!\n", reg, val);
		x3000_clk_dump("pll lock timeout");
		hang();
	}
}

static void cpccr_safe(void)
{
	unsigned int cpccr;
	unsigned int cpccr1;

	cpccr = CPCCR_SEL_SRC_EXCLK | CPCCR_SEL_LEPPLL_SCLKA |
		CPCCR_SEL_CPLL_SCLKA | CPCCR_CE_CPU | CPCCR_CE_LEP;
	cpccr1 = CPCCR1_SEL_H2PLL_SCLKA | CPCCR1_SEL_H1PLL_SCLKA |
		 CPCCR1_SEL_H0PLL_SCLKA | CPCCR1_CE_AHB2 |
		 CPCCR1_CE_AHB1 | CPCCR1_CE_AHB0;

	cpm_outl(cpccr, CPM_CPCCR);
	if (wait_cpcsr_clear(CPCSR_CDIV_BUSY | CPCSR_LEPDIV_BUSY,
			     CPCSR_SRC_MUX | CPCSR_CPU_MUX | CPCSR_LEP_MUX,
			     "CPCCR safe"))
		hang();

	cpm_outl(cpccr1, CPM_CPCCR1);
	if (wait_cpcsr_clear(CPCSR_H0DIV_BUSY | CPCSR_H1DIV_BUSY | CPCSR_H2DIV_BUSY,
			     CPCSR_AHB0_MUX | CPCSR_AHB1_MUX | CPCSR_AHB2_MUX,
			     "CPCCR1 safe"))
		hang();
}

static void cpccr_target(void)
{
	/* Move AHB/PCLK away from SCLK_A before SCLK_A is switched to APLL. */
	cpm_outl(CPCCR1_TARGET_VALUE, CPM_CPCCR1);
	if (wait_cpcsr_clear(CPCSR_H0DIV_BUSY | CPCSR_H1DIV_BUSY | CPCSR_H2DIV_BUSY,
			     CPCSR_AHB0_MUX | CPCSR_AHB1_MUX | CPCSR_AHB2_MUX,
			     "CPCCR1 target"))
		hang();

	cpm_outl(CPCCR_TARGET_VALUE, CPM_CPCCR);
	if (wait_cpcsr_clear(CPCSR_CDIV_BUSY | CPCSR_LEPDIV_BUSY,
			     CPCSR_SRC_MUX | CPCSR_CPU_MUX | CPCSR_LEP_MUX,
			     "CPCCR target"))
		hang();

	serial_debug("CPCCR:%x CPCCR1:%x CPCSR:%x\n",
		     cpm_inl(CPM_CPCCR), cpm_inl(CPM_CPCCR1), cpm_inl(CPM_CPCSR));
}

int pll_init(void)
{
	x3000_clk_dump("before safe");
	cpccr_safe();
	x3000_clk_dump("after safe");

	if (APLL_EN_VALUE)
		pll_set(CPM_CPAPCR);
	if (MPLL_EN_VALUE)
		pll_set(CPM_CPMPCR);
	if (EPLL_EN_VALUE)
		pll_set(CPM_CPEPCR);
	x3000_clk_dump("after pll set");

	cpccr_target();
	x3000_clk_dump("after target");

	return 0;
}
