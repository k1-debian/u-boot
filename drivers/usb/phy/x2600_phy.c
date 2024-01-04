#include "ingenic_phy.h"
#define OPCR_SPENDN0_BIT	7
#define OPCR_GATE_USBPHY_CLK_BIT	23
#define CLKGR0_GATE_OTG_CLK_BIT	4
#define SRBC_USB_SR	14
#define USBRDT_IDDIG_REG                23
#define USBRDT_IDDIG_EN			24

void otg_phy_init(enum otg_mode_t mode,unsigned extclk) {
	/*open clk*/
	cpm_clear_bit(CLKGR0_GATE_OTG_CLK_BIT, CPM_CLKGR0);
	cpm_clear_bit(OPCR_GATE_USBPHY_CLK_BIT, CPM_OPCR);

	cpm_set_bit(SRBC_USB_SR, CPM_SRBC);
	udelay(10);
	cpm_clear_bit(SRBC_USB_SR, CPM_SRBC);

	cpm_set_bit(OPCR_SPENDN0_BIT, CPM_OPCR);
	udelay(10);

	cpm_writel(0x00000000, CPM_USBPCR1);
	cpm_writel(0x80100000, CPM_USBPCR);
	udelay(800);
	cpm_writel(0x80000000, CPM_USBPCR);
	cpm_writel(0x30000000, CPM_USBPCR1);
	udelay(800);

	cpm_set_bit(USBRDT_IDDIG_EN, CPM_USBRDT);
	if(mode == HOST_ONLY_MODE)
		cpm_clear_bit(USBRDT_IDDIG_REG, CPM_USBRDT);
	else if(mode == DEVICE_ONLY_MODE)
		cpm_set_bit(USBRDT_IDDIG_REG, CPM_USBRDT);
}
