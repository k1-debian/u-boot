/*
 * Ingenic burn command
 *
 * Copyright (c) 2013 cli <cli@ingenic.cn>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>
#include <asm/usb_boot.h>

static int do_softburn(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
        enter_usb_boot_mode();
        return CMD_RET_SUCCESS;
}

U_BOOT_CMD(softburn, CONFIG_SYS_MAXARGS, 1, do_softburn,
	"Ingenic usb soft burn",
	"No params\n"
);
