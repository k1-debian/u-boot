/*
 * (C) Copyright 2010
 * Texas Instruments, <www.ti.com>
 *
 * Aneesh V <aneesh@ti.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <spl.h>
#include <asm/u-boot.h>
#include <nand.h>
#include <fat.h>
#include <version.h>
#include <i2c.h>
#include <image.h>
#include <malloc.h>
#include <linux/compiler.h>
#include <regulator.h>
#include "spl_rtos_argument.h"

DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_SYS_UBOOT_START
#define CONFIG_SYS_UBOOT_START	CONFIG_SYS_TEXT_BASE
#endif
#ifndef CONFIG_SYS_MONITOR_LEN
#define CONFIG_SYS_MONITOR_LEN	(200 * 1024)
#endif

u32 *boot_params_ptr = NULL;
struct spl_image_info spl_image;

/* Define board data structure */
static bd_t bdata __attribute__ ((section(".data")));

/*
 * Default function to determine if u-boot or the OS should
 * be started. This implementation always returns 1.
 *
 * Please implement your own board specific funcion to do this.
 *
 * RETURN
 * 0 to not start u-boot
 * positive if u-boot should start
 */
#ifdef CONFIG_SPL_OS_BOOT
__weak int spl_start_uboot(void)
{
	puts("SPL: Please implement spl_start_uboot() for your board\n");
	puts("SPL: Direct Linux boot not active!\n");
	return 1;
}
#endif

/*
 * Weak default function for board specific cleanup/preparation before
 * Linux boot. Some boards/platforms might not need it, so just provide
 * an empty stub here.
 */
__weak void spl_board_prepare_for_linux(void)
{
	/* Nothing to do! */
}

__weak char* spl_board_process_bootargs(char *arg)
{
	return arg;
}

extern char* spl_board_process_mem_bootargs(char *arg);

void spl_parse_image_header(const struct image_header *header)
{
	u32 header_size = sizeof(struct image_header);

	if (image_get_magic(header) == IH_MAGIC) {
		if (spl_image.flags & SPL_COPY_PAYLOAD_ONLY) {
			/*
			 * On some system (e.g. powerpc), the load-address and
			 * entry-point is located at address 0. We can't load
			 * to 0-0x40. So skip header in this case.
			 */
			spl_image.load_addr = image_get_load(header);
			spl_image.entry_point = image_get_ep(header);
			spl_image.size = image_get_data_size(header);
		} else {
			spl_image.entry_point = image_get_ep(header);
			/* Load including the header */
			spl_image.load_addr = image_get_load(header) -
				header_size;
			spl_image.size = image_get_data_size(header) +
				header_size;
		}
		spl_image.os = image_get_os(header);
		spl_image.name = image_get_name(header);
		debug("spl: payload image: %s load addr: 0x%x size: %d\n",
			spl_image.name, spl_image.load_addr, spl_image.size);
	} else {
		/* Signature not found - assume u-boot.bin */
		debug("mkimage signature not found - ih_magic = %x\n",
			header->ih_magic);
		/* Let's assume U-Boot will not be more than 200 KB */
		spl_image.size = CONFIG_SYS_MONITOR_LEN;
		spl_image.entry_point = CONFIG_SYS_UBOOT_START;
		spl_image.load_addr = CONFIG_SYS_TEXT_BASE;
		spl_image.os = IH_OS_U_BOOT;
		spl_image.name = "U-Boot";
	}
}

void spl_parse_image_info(const struct image_header *header, struct image_info *info)
{
	info->type = image_get_type(header);
	info->comp = image_get_comp(header);
	info->os = image_get_os(header);

	info->end = image_get_image_end(header);
	info->load = image_get_load(header);

	info->image_start = image_get_data(header);
	info->image_len = image_get_data_size(header);

	if (info->type == IH_TYPE_KERNEL_NOLOAD) {
		info->load = info->image_start;
		spl_image.entry_point += info->load;
	}
	info->start = (ulong)header;
}

__weak void __noreturn jump_to_image_no_args(struct spl_image_info *spl_image)
{
	typedef void __noreturn (*image_entry_noargs_t)(void);

	image_entry_noargs_t image_entry =
			(image_entry_noargs_t) spl_image->entry_point;

	debug("image entry point: 0x%X\n", spl_image->entry_point);
	image_entry();
}

#ifdef CONFIG_SPL_RAM_DEVICE
static void spl_ram_load_image(void)
{
	const struct image_header *header;

	/*
	 * Get the header.  It will point to an address defined by handoff
	 * which will tell where the image located inside the flash. For
	 * now, it will temporary fixed to address pointed by U-Boot.
	 */
	header = (struct image_header *)
		(CONFIG_SYS_TEXT_BASE -	sizeof(struct image_header));

	spl_parse_image_header(header);
}
#endif

extern char* spl_sfc_nand_load_image(void);
extern char* spl_sfc_nor_load_image(void);

void board_init_r(gd_t *dummy1, ulong dummy2)
{
	u32 boot_device;
	char *cmdargs = NULL;
	debug(">>spl:board_init_r()\n");

#ifdef CONFIG_SYS_SPL_MALLOC_START
	mem_malloc_init(CONFIG_SYS_SPL_MALLOC_START,
			CONFIG_SYS_SPL_MALLOC_SIZE);
#endif

#ifndef CONFIG_PPC
	/*
	 * timer_init() does not exist on PPC systems. The timer is initialized
	 * and enabled (decrementer) in interrupt_init() here.
	 */
	timer_init();
#endif

#ifdef CONFIG_SPL_BOARD_INIT
	spl_board_init();
#endif
	boot_device = spl_boot_device();
	debug("boot device - %d\n", boot_device);
#ifdef CONFIG_PALLADIUM
	spl_board_prepare_for_linux();
#endif
	switch (boot_device) {
#ifdef CONFIG_SPL_RAM_DEVICE
	case BOOT_DEVICE_RAM:
		spl_ram_load_image();
		break;
#endif
#if defined(CONFIG_SPL_MMC_SUPPORT) || defined(CONFIG_SPL_JZMMC_SUPPORT)
	case BOOT_DEVICE_MMC1:
	case BOOT_DEVICE_MMC2:
	case BOOT_DEVICE_MMC2_2:
		cmdargs = spl_mmc_load_image();
		break;
#endif
#if defined(CONFIG_SPL_NAND_SUPPORT) || defined(CONFIG_JZ_NAND_MGR)
	case BOOT_DEVICE_NAND:
		spl_nand_load_image();
		break;
#endif
#ifdef CONFIG_SPL_ONENAND_SUPPORT
	case BOOT_DEVICE_ONENAND:
		spl_onenand_load_image();
		break;
#endif
#ifdef CONFIG_SPL_NOR_SUPPORT
	case BOOT_DEVICE_NOR:
		spl_nor_load_image();
		break;
#endif
#ifdef CONFIG_SPL_YMODEM_SUPPORT
	case BOOT_DEVICE_UART:
		spl_ymodem_load_image();
		break;
#endif
#ifdef CONFIG_SPL_SPI_SUPPORT
	case BOOT_DEVICE_SPI:
		spl_spi_load_image();
		break;
#endif

#ifdef CONFIG_SPL_SPI_NAND
	case BOOT_DEVICE_SPI_NAND:
		spl_spi_nand_load_image();
		break;
#endif
#ifdef CONFIG_SPL_SFC_NOR
	case BOOT_DEVICE_SFC_NOR:
		cmdargs = spl_sfc_nor_load_image();
		break;
#endif

#ifdef CONFIG_SPL_SFC_NAND
	case BOOT_DEVICE_SFC_NAND:
		cmdargs = spl_sfc_nand_load_image();
		break;
#endif

#ifdef CONFIG_SPL_ETH_SUPPORT
	case BOOT_DEVICE_CPGMAC:
#ifdef CONFIG_SPL_ETH_DEVICE
		spl_net_load_image(CONFIG_SPL_ETH_DEVICE);
#else
		spl_net_load_image(NULL);
#endif
		break;
#endif
#ifdef CONFIG_SPL_USBETH_SUPPORT
	case BOOT_DEVICE_USBETH:
		spl_net_load_image("usb_ether");
		break;
#endif
	default:
		debug("SPL: Un-supported Boot Device\n");
		hang();
	}

	switch (spl_image.os) {
	case IH_OS_U_BOOT:
		debug("Jumping to U-Boot\n");
		break;
#ifdef CONFIG_SPL_OS_BOOT
	case IH_OS_LINUX:
		debug("Jumping to Linux\n");
		spl_board_prepare_for_linux();

		cmdargs = cmdargs ? cmdargs : CONFIG_SYS_SPL_ARGS_ADDR;
		cmdargs = spl_board_process_bootargs(cmdargs);
#if defined(CONFIG_SPL_AUTO_PROBE_ARGS_MEM) && !defined(CONFIG_SPL_RTOS_LOAD_KERNEL)
		cmdargs = spl_board_process_mem_bootargs(cmdargs);
#endif
		debug("get cmdargs: %s.\n", cmdargs);
		jump_to_image_linux((void *)cmdargs);
#endif
	case IH_OS_ALIOS:
		debug("Jumping to Alios\n");
		break;
	default:
		debug("Unsupported OS image.. Jumping nevertheless..\n");
	}

	jump_to_image_no_args(&spl_image);
}

/*
 * This requires UART clocks to be enabled.  In order for this to work the
 * caller must ensure that the gd pointer is valid.
 */
#ifdef CONFIG_GPIO_SPI_TO_UART
extern int gpio_spi_to_uart_init(void);
#endif
#ifdef CONFIG_GPIO_SPI_TO_UART2
extern int gpio_spi_to_uart_init2(void);
#endif
void preloader_console_init(void)
{
	gd->bd = &bdata;
#ifndef CONFIG_BURNER
	gd->baudrate = CONFIG_BAUDRATE;
#else
	gd->baudrate = gd->arch.gi->baud_rate;
#endif
#ifdef CONFIG_PALLADIUM
	gd->baudrate = 3750000;
#endif
#if defined CONFIG_GPIO_SPI_TO_UART
	gpio_spi_to_uart_init();
#elif defined CONFIG_GPIO_SPI_TO_UART2
	gpio_spi_to_uart_init2();
#else
	serial_init();		/* serial communications setup */
#endif

	gd->have_console = 1;
#ifndef CONFIG_DDR_DRVODT_DEBUG
	serial_debug("\nU-Boot SPL " PLAIN_VERSION " (" U_BOOT_DATE " - " \
			U_BOOT_TIME ")\n");
#endif
#ifdef CONFIG_SPL_DISPLAY_PRINT
	spl_display_print();
#endif
}

void spl_regulator_set(void)
{
#ifdef CONFIG_SPL_CORE_VOLTAGE
	spl_regulator_init();
	debug("Set core voltage:%dmv\n", CONFIG_SPL_CORE_VOLTAGE);
	spl_regulator_set_voltage(REGULATOR_CORE, CONFIG_SPL_CORE_VOLTAGE);
#endif
#ifdef CONFIG_SPL_MEM_VOLTAGE
	debug("Set mem voltage:%dmv\n", CONFIG_SPL_MEM_VOLTAGE);
	spl_regulator_set_voltage(REGULATOR_MEM, CONFIG_SPL_MEM_VOLTAGE);
#endif
}
