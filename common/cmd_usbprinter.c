/*
 * Ingenic usb printer Command Explain CMD
 *
 *  Copyright (C) 2013 Ingenic Semiconductor Co., LTD.
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

extern int usb_gprinter_register(const char *type);
extern int usb_gadget_handle_interrupts(void);
extern int printer_get_connect_status(void);
extern int printer_write(uint8_t *buffer, uint32_t len);
extern int printer_read(uint8_t *buffer, uint32_t len);

static unsigned char usb_test_buf[8192];
static char *usb_test_char = "hello word!\r\n";

static int do_gprinter(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *s = "gprinter";
	int len, i;

	if (argc > 1)
		return CMD_RET_USAGE;

	usb_gprinter_register(s);

	while(1)
	{
		usb_gadget_handle_interrupts();
		if(printer_get_connect_status()){
			memset(usb_test_buf, 0, sizeof(usb_test_buf));
			len = printer_read(usb_test_buf, sizeof(usb_test_buf));
			if(len > 0){
				printf("usb printer read %d bytes data\n",len);
				for(i = 0; i < len; i++)
					printf("%c", usb_test_buf[i]);
			}
//			printer_write(usb_test_char, strlen(usb_test_char));
		}
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	gprinter, 1, 1, do_gprinter,
	"enter gprinter mode",
	"enter gprinter mode"
);



