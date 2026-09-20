/*
 * Ingenic X3000 base setup code.
 */

#include <common.h>
#include <nand.h>
#include <net.h>
#include <netdev.h>
#include <asm/gpio.h>
#include <asm/arch/cpm.h>
#include <asm/arch/clk.h>
#include <asm/arch/mmc.h>

int board_early_init_f(void)
{
	return 0;
}

int board_early_init_r(void)
{
	return 0;
}

int misc_init_r(void)
{
	return 0;
}

#ifdef CONFIG_MMC
extern void jz_mmc_init(void);
int board_mmc_init(bd_t *bd)
{
	jz_mmc_init();
	return 0;
}
#endif

#ifdef CONFIG_SYS_NAND_SELF_INIT
void board_nand_init(void)
{
}
#endif

int board_eth_init(bd_t *bis)
{
	return 0;
}

int checkboard(void)
{
	puts("Board: x3000_base (Ingenic XBurst2 X3000 SoC)\n");
	return 0;
}

#ifdef CONFIG_SPL_BUILD
void spl_board_init(void)
{
}

#endif /* CONFIG_SPL_BUILD */
