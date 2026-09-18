#include <stdio.h>
#include <config.h>
#include <asm/arch/cpm.h>
#include <asm/arch/clk.h>

#define M(x) (x * 1000 * 1000)

#define X3000_PLL_FREF_MIN_MHZ	1
#define X3000_PLL_FREF_MAX_MHZ	1200
#define X3000_PLL_FVCO_MIN_MHZ	950
#define X3000_PLL_FVCO_MAX_MHZ	3800

#ifndef CONFIG_SYS_AHB0_FREQ
#define CONFIG_SYS_AHB0_FREQ M(300)
#endif

#ifndef CONFIG_SYS_AHB1_FREQ
#define CONFIG_SYS_AHB1_FREQ CONFIG_SYS_AHB0_FREQ
#endif

#ifndef CONFIG_SYS_AHB2_FREQ
#define CONFIG_SYS_AHB2_FREQ M(300)
#endif

#ifndef CONFIG_SYS_PCLK_FREQ
#define CONFIG_SYS_PCLK_FREQ (CONFIG_SYS_AHB2_FREQ / 2)
#endif

#ifndef CONFIG_SYS_LEP_FREQ
#define CONFIG_SYS_LEP_FREQ CONFIG_SYS_AHB2_FREQ
#endif

#define out_error(fmt, y...) do {						\
		printf("#error " fmt, ##y);					\
		printf("please check %s %d\n", __FILE__, __LINE__);		\
	} while (0)

struct pll_control {
	unsigned int pllm:12;
	unsigned int plln:6;
	unsigned int pllod1:3;
	unsigned int pllod0:3;
};

struct div_setting {
	int cdiv;
	int l2div;
	int lepdiv;
	int h0div;
	int h1div;
	int h2div;
	int pdiv;
	unsigned int sel_src;
	unsigned int sel_cpll;
	unsigned int sel_leppll;
	unsigned int sel_h0pll;
	unsigned int sel_h1pll;
	unsigned int sel_h2pll;
};

static unsigned int pll_freq_by_id(unsigned int pll)
{
	switch (pll) {
	case APLL:
		return CONFIG_SYS_APLL_FREQ;
	case MPLL:
		return CONFIG_SYS_MPLL_FREQ;
	case EPLL:
		return CONFIG_SYS_EPLL_FREQ;
	default:
		return 0;
	}
}

static unsigned int gen_pll_register(unsigned int extal, unsigned int pll_rate,
				     struct pll_control *p)
{
	int fin = extal / 1000000;
	int fout = pll_rate / 1000000;
	int n, m, od1;
	int fref, fvco;
	int od0 = 1;

	if (!pll_rate)
		return 0;

	for (n = 1; n < 64; n++) {
		for (m = 16; m < 2048; m++) {
			for (od1 = 1; od1 < 8; od1++) {
				fref = fin / n;
				if ((fin % n) ||
				    (fref < X3000_PLL_FREF_MIN_MHZ) ||
				    (fref > X3000_PLL_FREF_MAX_MHZ))
					continue;

				fvco = fout * od0 * od1;
				if ((fref * m != fvco) ||
				    (fvco < X3000_PLL_FVCO_MIN_MHZ) ||
				    (fvco > X3000_PLL_FVCO_MAX_MHZ))
					continue;

				p->pllm = m;
				p->plln = n;
				p->pllod0 = od0;
				p->pllod1 = od1;
				return 1;
			}
		}
	}

	out_error("no adjust parameter to the fout:%dM fin:%dM check %s %d\n",
		  fout, fin, __FILE__, __LINE__);
	return 0;
}

static int div_exact(unsigned int src, unsigned int target, const char *name)
{
	unsigned int div;

	if (!src || !target || src < target) {
		out_error("%s source[%u] target[%u] invalid\n", name, src, target);
		return 0;
	}

	div = src / target;
	if (src % target) {
		out_error("%s source[%u] should be divided by target[%u]\n",
			  name, src, target);
		return 0;
	}

	if (div < 1 || div > 16) {
		out_error("%s div[%u] is out of range\n", name, div);
		return 0;
	}

	return div - 1;
}

static int div_min(unsigned int src, unsigned int max, const char *name)
{
	unsigned int div = 1;

	if (!src || !max) {
		out_error("%s source[%u] max[%u] invalid\n", name, src, max);
		return 0;
	}

	while (src / div > max)
		div++;

	if (div > 16) {
		out_error("%s div[%u] is out of range\n", name, div);
		return 0;
	}

	return div - 1;
}

static unsigned int sclka_sel(unsigned int pll)
{
	switch (pll) {
	case APLL:
		return CPCCR_SEL_SRC_APLL;
	default:
		out_error("X3000 SCLK_A supports APLL only in this boot path\n");
		return CPCCR_SEL_SRC_EXCLK;
	}
}

static unsigned int cpll_sel(unsigned int pll)
{
	switch (pll) {
	case APLL:
		return CPCCR_SEL_CPLL_SCLKA;
	case MPLL:
		return CPCCR_SEL_CPLL_MPLL;
	default:
		out_error("X3000 CPU pll[%u] unsupported\n", pll);
		return CPCCR_SEL_CPLL_SCLKA;
	}
}

static unsigned int lep_sel(unsigned int pll)
{
	switch (pll) {
	case APLL:
		return CPCCR_SEL_LEPPLL_SCLKA;
	case MPLL:
		return CPCCR_SEL_LEPPLL_MPLL;
	default:
		out_error("X3000 LEP pll[%u] unsupported\n", pll);
		return CPCCR_SEL_LEPPLL_SCLKA;
	}
}

static unsigned int ahb_sel(unsigned int pll, unsigned int shift)
{
	switch (pll) {
	case APLL:
		return 1 << shift;
	case MPLL:
		return 2 << shift;
	default:
		out_error("X3000 AHB pll[%u] unsupported\n", pll);
		return 1 << shift;
	}
}

static void gen_sys_div(struct div_setting *div)
{
	unsigned int cpu_src = pll_freq_by_id(CONFIG_CPU_SEL_PLL);
	unsigned int periph_src = pll_freq_by_id(MPLL);
	unsigned int pclk;

	div->sel_src = sclka_sel(APLL);
	div->sel_cpll = cpll_sel(CONFIG_CPU_SEL_PLL);
	div->sel_leppll = lep_sel(MPLL);
	div->sel_h0pll = ahb_sel(MPLL, 24);
	div->sel_h1pll = ahb_sel(MPLL, 26);
	div->sel_h2pll = ahb_sel(MPLL, 28);

	div->cdiv = div_exact(cpu_src, CONFIG_SYS_CPU_FREQ, "cpu");
	div->l2div = div_min(cpu_src, CONFIG_SYS_CPU_FREQ / 2, "l2");
	div->lepdiv = div_min(periph_src, CONFIG_SYS_LEP_FREQ, "lep");
	div->h0div = div_min(periph_src, CONFIG_SYS_AHB0_FREQ, "ahb0");
	div->h1div = div_min(periph_src, CONFIG_SYS_AHB1_FREQ, "ahb1");
	div->h2div = div_min(periph_src, CONFIG_SYS_AHB2_FREQ, "ahb2");
	div->pdiv = div_exact(periph_src, CONFIG_SYS_PCLK_FREQ, "pclk");

	pclk = periph_src / (div->pdiv + 1);
	if ((periph_src / (div->h2div + 1)) != pclk &&
	    (periph_src / (div->h2div + 1)) != (pclk * 2))
		out_error("ahb2[%u] must be 1 or 2 times pclk[%u]\n",
			  periph_src / (div->h2div + 1), pclk);
}

static void file_head_print(void)
{
	printf("/*\n");
	printf(" * DO NOT MODIFY.\n");
	printf(" *\n");
	printf(" * This file was generated by x3000 pll_params_creator\n");
	printf(" *\n");
	printf(" */\n\n");
	printf("#ifndef __PLL_REG_VALUES_H__\n");
	printf("#define __PLL_REG_VALUES_H__\n\n");
}

static void file_end_print(void)
{
	printf("\n#endif /* __PLL_REG_VALUES_H__ */\n");
}

int main(int argc, char *argv[])
{
	struct pll_control apll_ctrl = {0};
	struct pll_control mpll_ctrl = {0};
	struct pll_control epll_ctrl = {0};
	struct div_setting div = {0};
	int apll, mpll, epll;
	unsigned int cpccr, cpccr1;

	apll = gen_pll_register(CONFIG_SYS_EXTAL, CONFIG_SYS_APLL_FREQ, &apll_ctrl);
	mpll = gen_pll_register(CONFIG_SYS_EXTAL, CONFIG_SYS_MPLL_FREQ, &mpll_ctrl);
	epll = gen_pll_register(CONFIG_SYS_EXTAL, CONFIG_SYS_EPLL_FREQ, &epll_ctrl);
	gen_sys_div(&div);

	cpccr = div.sel_src | div.sel_leppll | div.sel_cpll |
		CPCCR_CE_CPU | CPCCR_CE_LEP |
		((div.lepdiv & CPCCR_DIV_MASK) << CPCCR_LEPDIV_SHIFT) |
		((div.l2div & CPCCR_DIV_MASK) << CPCCR_L2DIV_SHIFT) |
		((div.cdiv & CPCCR_DIV_MASK) << CPCCR_CDIV_SHIFT);
	cpccr1 = div.sel_h2pll | div.sel_h1pll | div.sel_h0pll |
		 CPCCR1_CE_AHB2 | CPCCR1_CE_AHB1 | CPCCR1_CE_AHB0 |
		 ((div.pdiv & CPCCR_DIV_MASK) << CPCCR1_PDIV_SHIFT) |
		 ((div.h2div & CPCCR_DIV_MASK) << CPCCR1_H2DIV_SHIFT) |
		 ((div.h1div & CPCCR_DIV_MASK) << CPCCR1_H1DIV_SHIFT) |
		 ((div.h0div & CPCCR_DIV_MASK) << CPCCR1_H0DIV_SHIFT);

	file_head_print();
	printf("#define APLL_M_VALUE \t\t 0x%08x\n", apll_ctrl.pllm);
	printf("#define APLL_N_VALUE \t\t 0x%08x\n", apll_ctrl.plln);
	printf("#define APLL_OD0_VALUE \t\t 0x%08x\n", apll_ctrl.pllod0);
	printf("#define APLL_OD1_VALUE \t\t 0x%08x\n", apll_ctrl.pllod1);
	printf("#define APLL_EN_VALUE \t\t 0x%08x\n", apll ? 1 : 0);

	printf("#define MPLL_M_VALUE \t\t 0x%08x\n", mpll_ctrl.pllm);
	printf("#define MPLL_N_VALUE \t\t 0x%08x\n", mpll_ctrl.plln);
	printf("#define MPLL_OD0_VALUE \t\t 0x%08x\n", mpll_ctrl.pllod0);
	printf("#define MPLL_OD1_VALUE \t\t 0x%08x\n", mpll_ctrl.pllod1);
	printf("#define MPLL_EN_VALUE \t\t 0x%08x\n", mpll ? 1 : 0);

	printf("#define EPLL_M_VALUE \t\t 0x%08x\n", epll_ctrl.pllm);
	printf("#define EPLL_N_VALUE \t\t 0x%08x\n", epll_ctrl.plln);
	printf("#define EPLL_OD0_VALUE \t\t 0x%08x\n", epll_ctrl.pllod0);
	printf("#define EPLL_OD1_VALUE \t\t 0x%08x\n", epll_ctrl.pllod1);
	printf("#define EPLL_EN_VALUE \t\t 0x%08x\n", epll ? 1 : 0);

	printf("#define CDIV_REG_VALUE\t\t 0x%08x\n", div.cdiv);
	printf("#define L2DIV_REG_VALUE\t\t 0x%08x\n", div.l2div);
	printf("#define LEPDIV_REG_VALUE\t 0x%08x\n", div.lepdiv);
	printf("#define H0DIV_REG_VALUE\t\t 0x%08x\n", div.h0div);
	printf("#define H1DIV_REG_VALUE\t\t 0x%08x\n", div.h1div);
	printf("#define H2DIV_REG_VALUE\t\t 0x%08x\n", div.h2div);
	printf("#define PDIV_REG_VALUE\t\t 0x%08x\n", div.pdiv);
	printf("#define CPCCR_TARGET_VALUE\t 0x%08x\n", cpccr);
	printf("#define CPCCR1_TARGET_VALUE\t 0x%08x\n", cpccr1);
	file_end_print();

	return 0;
}
