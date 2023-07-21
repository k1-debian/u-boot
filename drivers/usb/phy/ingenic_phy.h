#include <asm/io.h>
#include <asm/arch/clk.h>
#include <asm/arch/cpm.h>
#define phy_inl(off)		readl(OTGPHY_BASE + (off))
#define phy_outl(val,off)	writel(val,OTGPHY_BASE + (off))
void otg_phy_init(enum otg_mode_t mode,unsigned extclk);
