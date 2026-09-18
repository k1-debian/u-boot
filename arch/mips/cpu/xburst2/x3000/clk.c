#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>
#include <asm/arch/clk.h>
#include <generated/clk_reg_values.h>

DECLARE_GLOBAL_DATA_PTR;

#define X3000_CLK_TIMEOUT	0x10000

struct clk_cgu_setting cgusetting[] = CGU_REG_VALUE;

static int is_msc_clk(int clk_id)
{
	return clk_id == MSC0 || clk_id == MSC1 || clk_id == MSC2;
}

static int clk_wait_busy(struct clk_cgu_setting *cgu, int clk_id)
{
	unsigned int timeout = X3000_CLK_TIMEOUT;

	if (!cgu->busy)
		return 0;

	while ((readl(cgu->addr) & (1 << cgu->busy)) && --timeout)
		;

	if (!timeout) {
		serial_debug("wait clk %d timeout, reg=%08x\n",
			     clk_id, readl(cgu->addr));
		return -1;
	}

	return 0;
}

static void clk_wait_busy_or_hang(struct clk_cgu_setting *cgu, int clk_id)
{
	if (clk_wait_busy(cgu, clk_id))
		hang();
}

static unsigned int x3000_uart_gate(unsigned int uart_idx)
{
	switch (uart_idx) {
	case 0:
		return CPM_CLKGR_UART0;
	case 1:
		return CPM_CLKGR_UART1;
	case 2:
		return CPM_CLKGR_UART2;
	case 3:
		return CPM_CLKGR_UART3;
	case 4:
		return CPM_CLKGR_UART4;
	case 5:
		return CPM_CLKGR_UART5;
	case 6:
		return CPM_CLKGR_UART6;
	case 7:
		return CPM_CLKGR_UART7;
	default:
		return CPM_CLKGR_UART0;
	}
}

void clk_prepare(void)
{
	int i;
	unsigned int regval;
	unsigned int size = ARRAY_SIZE(cgusetting);

	for (i = 0; i < size; i++) {
		if (!cgusetting[i].valid || is_msc_clk(i))
			continue;

		regval = readl(cgusetting[i].addr);
		if (cgusetting[i].busy) {
			regval |= cgusetting[i].val;
			writel(regval, cgusetting[i].addr);
			clk_wait_busy_or_hang(&cgusetting[i], i);
		}
	}
}

static unsigned int pll_get_rate(int pll)
{
	unsigned int reg, m, n, od1, od0;
	unsigned int rate;

	switch (pll) {
	case APLL:
		reg = cpm_inl(CPM_CPAPCR);
		break;
	case MPLL:
		reg = cpm_inl(CPM_CPMPCR);
		break;
	case EPLL:
		reg = cpm_inl(CPM_CPEPCR);
		break;
	case EXCLK:
		return CONFIG_SYS_EXTAL;
	default:
		return 0;
	}

	m = (reg >> 20) & 0xfff;
	n = (reg >> 14) & 0x3f;
	od1 = (reg >> 11) & 0x7;
	od0 = (reg >> 8) & 0x7;
	if (!m || !n || !od1 || !od0)
		return 0;

	rate = CONFIG_SYS_EXTAL / 1000 / 1000;
	rate = rate * m / n / (od1 * od0);

	return rate * 1000 * 1000;
}

static unsigned int sclka_get_rate(void)
{
	unsigned int cpccr = cpm_inl(CPM_CPCCR);

	switch (cpccr & CPCCR_SEL_SRC_MASK) {
	case CPCCR_SEL_SRC_EXCLK:
		return CONFIG_SYS_EXTAL;
	case CPCCR_SEL_SRC_APLL:
		return pll_get_rate(APLL);
	default:
		return 0;
	}
}

static unsigned int get_cclk_rate(void)
{
	unsigned int cpccr = cpm_inl(CPM_CPCCR);

	switch (cpccr & CPCCR_SEL_CPLL_MASK) {
	case CPCCR_SEL_CPLL_SCLKA:
		return sclka_get_rate() / ((cpccr & CPCCR_DIV_MASK) + 1);
	case CPCCR_SEL_CPLL_MPLL:
		return pll_get_rate(MPLL) / ((cpccr & 0xf) + 1);
	default:
		return 0;
	}
}

static unsigned int get_cgu_parent_rate(unsigned int clk_id, unsigned int regval)
{
	switch ((regval >> 30) & 3) {
	case 0:
		if (clk_id == DDR)
			return 0;
		return sclka_get_rate();
	case 1:
		if (clk_id == DDR)
			return sclka_get_rate();
		return pll_get_rate(MPLL);
	case 2:
		if (clk_id == DDR)
			return pll_get_rate(MPLL);
		if (is_msc_clk(clk_id))
			return CONFIG_SYS_EXTAL;
		return pll_get_rate(EPLL);
	default:
		return 0;
	}
}

static unsigned int get_cgu_rate(unsigned int clk_id)
{
	struct clk_cgu_setting *cgu = &cgusetting[clk_id];
	unsigned int pll_rate;
	unsigned int regval;

	if (!cgu->valid)
		return 0;

	regval = readl(cgu->addr);
	pll_rate = get_cgu_parent_rate(clk_id, regval);

	if (!pll_rate)
		return 0;

	if (is_msc_clk(clk_id))
		return pll_rate / (((regval & cgu->div_mask) + 1) * 4);

	return pll_rate / ((regval & cgu->div_mask) + 1);
}

unsigned int cpm_get_h2clk(void)
{
	unsigned int cpccr1 = cpm_inl(CPM_CPCCR1);
	unsigned int div = ((cpccr1 >> CPCCR1_H2DIV_SHIFT) & CPCCR_DIV_MASK) + 1;

	switch (cpccr1 & CPCCR1_SEL_H2PLL_MASK) {
	case CPCCR1_SEL_H2PLL_SCLKA:
		return sclka_get_rate() / div;
	case CPCCR1_SEL_H2PLL_MPLL:
		return pll_get_rate(MPLL) / div;
	default:
		return 0;
	}
}

unsigned int clk_get_rate(int clk)
{
	switch (clk) {
	case APLL:
	case MPLL:
	case EPLL:
	case EXCLK:
		return pll_get_rate(clk);
	case CPU:
		return get_cclk_rate();
	case H2CLK:
		return cpm_get_h2clk();
	case 0 ... (CGU_CNT - 1):
		return get_cgu_rate(clk);
	default:
		return 0;
	}
}

void clk_set_rate(int clk_id, unsigned long rate)
{
	struct clk_cgu_setting *cgu;
	unsigned int pll_rate, regval, ratio, cdr, sel_val;

	if (clk_id < 0 || clk_id >= CGU_CNT) {
		serial_debug("x3000 clk %d is out of range\n", clk_id);
		return;
	}

	cgu = &cgusetting[clk_id];
	if (!cgu->valid) {
		serial_debug("x3000 clk %d is unsupported\n", clk_id);
		return;
	}

	pll_rate = pll_get_rate(cgu->sel_src);
	if (!pll_rate || !rate) {
		serial_debug("x3000 clk %d invalid parent rate=%u target=%lu\n",
			     clk_id, pll_rate, rate);
		return;
	}

	regval = readl(cgu->addr);
	sel_val = cgu->sel_val;

	if (is_msc_clk(clk_id)) {
		if (rate > (CONFIG_SYS_EXTAL / 4)) {
			regval &= ~MSCCDR_EXCK_E;
		} else {
			/*
			 * 切到 EXCLK 之前必须使能 MSC 的外部时钟。注意该位在
			 * CPM_MSC0CDR 里（MSC 组共用），与配的是哪个 MSC 无关：
			 * X3000 BootROM 的 msc_clk_div_set() 也是固定写
			 * CPM_MSC0CDR | MSCCDR_EXCK_E。写在 MSCxCDR 上读回永远是
			 * 0，切换序列等不到 EXCLK 就绪，BUSY 一直不落（现场现象：
			 * MSC2CDR 读回 MPCS=10、CE=1、BUSY=1、bit21 为 0）。
			 */
			cpm_outl(cpm_inl(CPM_MSC0CDR) | MSCCDR_EXCK_E,
				 CPM_MSC0CDR);
			regval = readl(cgu->addr);
			sel_val = MSCCDR_MPCS_EXCLK;
			pll_rate = CONFIG_SYS_EXTAL;
		}
		ratio = (pll_rate + rate - 1) / rate;
		cdr = (ratio % 4) ? ratio / 4 : (ratio / 4 - 1);
	} else {
		cdr = (pll_rate + rate - 1) / rate - 1;
	}

	if (cdr > cgu->div_mask) {
		serial_debug("x3000 clk %d divider overflow: parent=%u target=%lu cdr=%u mask=%08x\n",
			     clk_id, pll_rate, rate, cdr, cgu->div_mask);
		hang();
	}

	regval &= ~((1 << cgu->stop) | cgu->div_mask | (3 << 30));
	regval |= (1 << cgu->ce) | cdr;
	regval |= sel_val;
	writel(regval, cgu->addr);
	clk_wait_busy_or_hang(cgu, clk_id);

	/* 参考实现切换完成后把 CE（改频触发位）清掉，恢复静态状态。 */
	if (is_msc_clk(clk_id)) {
		regval &= ~(1 << cgu->ce);
		writel(regval, cgu->addr);
		clk_wait_busy(cgu, clk_id);
	}
}

void clk_init(void)
{
	unsigned int gate0 = CPM_CLKGR_DDR;

#ifdef CONFIG_JZ_MMC_MSC0
	gate0 |= CPM_CLKGR_MSC0;
#endif
#ifdef CONFIG_JZ_MMC_MSC1
	gate0 |= CPM_CLKGR_MSC1;
#endif
#ifdef CONFIG_JZ_MMC_MSC2
	gate0 |= CPM_CLKGR_MSC2;
#endif
#ifdef CONFIG_JZ_SFC
	gate0 |= CPM_CLKGR_SFC;
#endif

	if (gate0)
		cpm_outl(gate0, CPM_CLKGR0_CLR);
}

void enable_uart_clk(void)
{
	unsigned int gate = x3000_uart_gate(gd->arch.gi->uart_idx);

	if (gate)
		cpm_outl(gate, CPM_CLKGR1_CLR);
}
