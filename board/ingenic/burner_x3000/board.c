/*
 * Ingenic X3000 burner setup code
 *
 * Copyright (c) 2026 Ingenic Semiconductor Co.,Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <common.h>
#include <nand.h>
#include <serial.h>

DECLARE_GLOBAL_DATA_PTR;

extern void burner_param_info(void);
extern int jz_udc_probe(void);
extern void jz_mmc_init(void);

int board_early_init_f(void)
{
	burner_param_info();
	serial_init();
	return 0;
}

int misc_init_r(void)
{
	return 0;
}

void board_usb_init(void)
{
	jz_udc_probe();
}

int board_mmc_init(bd_t *bd)
{
	jz_mmc_init();
	return 0;
}

#ifdef CONFIG_SYS_NAND_SELF_INIT
void board_nand_init(void)
{
}
#endif

int checkboard(void)
{
	return 0;
}
