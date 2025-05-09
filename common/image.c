/*
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

#ifndef USE_HOSTCC
#include <common.h>
#include <watchdog.h>

#ifdef CONFIG_SHOW_BOOT_PROGRESS
#include <status_led.h>
#endif

#ifdef CONFIG_HAS_DATAFLASH
#include <dataflash.h>
#endif

#ifdef CONFIG_LOGBUFFER
#include <logbuff.h>
#endif

#include <rtc.h>

#include <environment.h>
#include <image.h>

#if defined(CONFIG_FIT) || defined(CONFIG_OF_LIBFDT)
#include <libfdt.h>
#include <fdt_support.h>
#endif

#include <u-boot/md5.h>
#include <sha1.h>
#include <asm/errno.h>
#include <asm/io.h>

#ifdef CONFIG_CMD_BDI
extern int do_bdinfo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
#endif

DECLARE_GLOBAL_DATA_PTR;

static const image_header_t *image_get_ramdisk(ulong rd_addr, uint8_t arch,
						int verify);
#else
#include "mkimage.h"
#include <u-boot/md5.h>
#include <time.h>
#include <image.h>
#endif /* !USE_HOSTCC*/

#include <u-boot/crc.h>

#ifndef CONFIG_SYS_BARGSIZE
#define CONFIG_SYS_BARGSIZE 512
#endif

static const table_entry_t uimage_arch[] = {
	{	IH_ARCH_INVALID,	NULL,		"Invalid ARCH",	},
	{	IH_ARCH_ALPHA,		"alpha",	"Alpha",	},
	{	IH_ARCH_ARM,		"arm",		"ARM",		},
	{	IH_ARCH_I386,		"x86",		"Intel x86",	},
	{	IH_ARCH_IA64,		"ia64",		"IA64",		},
	{	IH_ARCH_M68K,		"m68k",		"M68K",		},
	{	IH_ARCH_MICROBLAZE,	"microblaze",	"MicroBlaze",	},
	{	IH_ARCH_MIPS,		"mips",		"MIPS",		},
	{	IH_ARCH_MIPS64,		"mips64",	"MIPS 64 Bit",	},
	{	IH_ARCH_NIOS2,		"nios2",	"NIOS II",	},
	{	IH_ARCH_PPC,		"powerpc",	"PowerPC",	},
	{	IH_ARCH_PPC,		"ppc",		"PowerPC",	},
	{	IH_ARCH_S390,		"s390",		"IBM S390",	},
	{	IH_ARCH_SH,		"sh",		"SuperH",	},
	{	IH_ARCH_SPARC,		"sparc",	"SPARC",	},
	{	IH_ARCH_SPARC64,	"sparc64",	"SPARC 64 Bit",	},
	{	IH_ARCH_BLACKFIN,	"blackfin",	"Blackfin",	},
	{	IH_ARCH_AVR32,		"avr32",	"AVR32",	},
	{	IH_ARCH_NDS32,		"nds32",	"NDS32",	},
	{	IH_ARCH_OPENRISC,	"or1k",		"OpenRISC 1000",},
	{	IH_ARCH_SANDBOX,	"sandbox",	"Sandbox",	},
	{	-1,			"",		"",		},
};

static const table_entry_t uimage_os[] = {
	{	IH_OS_INVALID,	NULL,		"Invalid OS",		},
	{	IH_OS_LINUX,	"linux",	"Linux",		},
#if defined(CONFIG_LYNXKDI) || defined(USE_HOSTCC)
	{	IH_OS_LYNXOS,	"lynxos",	"LynxOS",		},
#endif
	{	IH_OS_NETBSD,	"netbsd",	"NetBSD",		},
	{	IH_OS_OSE,	"ose",		"Enea OSE",		},
	{	IH_OS_PLAN9,	"plan9",	"Plan 9",		},
	{	IH_OS_RTEMS,	"rtems",	"RTEMS",		},
	{	IH_OS_U_BOOT,	"u-boot",	"U-Boot",		},
#if defined(CONFIG_CMD_ELF) || defined(USE_HOSTCC)
	{	IH_OS_QNX,	"qnx",		"QNX",			},
	{	IH_OS_VXWORKS,	"vxworks",	"VxWorks",		},
#endif
#if defined(CONFIG_INTEGRITY) || defined(USE_HOSTCC)
	{	IH_OS_INTEGRITY,"integrity",	"INTEGRITY",		},
#endif
#ifdef USE_HOSTCC
	{	IH_OS_4_4BSD,	"4_4bsd",	"4_4BSD",		},
	{	IH_OS_DELL,	"dell",		"Dell",			},
	{	IH_OS_ESIX,	"esix",		"Esix",			},
	{	IH_OS_FREEBSD,	"freebsd",	"FreeBSD",		},
	{	IH_OS_IRIX,	"irix",		"Irix",			},
	{	IH_OS_NCR,	"ncr",		"NCR",			},
	{	IH_OS_OPENBSD,	"openbsd",	"OpenBSD",		},
	{	IH_OS_PSOS,	"psos",		"pSOS",			},
	{	IH_OS_SCO,	"sco",		"SCO",			},
	{	IH_OS_SOLARIS,	"solaris",	"Solaris",		},
	{	IH_OS_SVR4,	"svr4",		"SVR4",			},
#endif
	{	-1,		"",		"",			},
};

static const table_entry_t uimage_type[] = {
	{	IH_TYPE_AISIMAGE,   "aisimage",   "Davinci AIS image",},
	{	IH_TYPE_FILESYSTEM, "filesystem", "Filesystem Image",	},
	{	IH_TYPE_FIRMWARE,   "firmware",	  "Firmware",		},
	{	IH_TYPE_FLATDT,     "flat_dt",    "Flat Device Tree",	},
	{	IH_TYPE_KERNEL,	    "kernel",	  "Kernel Image",	},
	{	IH_TYPE_KERNEL_NOLOAD, "kernel_noload",  "Kernel Image (no loading done)", },
	{	IH_TYPE_KWBIMAGE,   "kwbimage",   "Kirkwood Boot Image",},
	{	IH_TYPE_IMXIMAGE,   "imximage",   "Freescale i.MX Boot Image",},
	{	IH_TYPE_INVALID,    NULL,	  "Invalid Image",	},
	{	IH_TYPE_MULTI,	    "multi",	  "Multi-File Image",	},
	{	IH_TYPE_OMAPIMAGE,  "omapimage",  "TI OMAP SPL With GP CH",},
	{	IH_TYPE_PBLIMAGE,   "pblimage",   "Freescale PBL Boot Image",},
	{	IH_TYPE_RAMDISK,    "ramdisk",	  "RAMDisk Image",	},
	{	IH_TYPE_SCRIPT,     "script",	  "Script",		},
	{	IH_TYPE_STANDALONE, "standalone", "Standalone Program", },
	{	IH_TYPE_UBLIMAGE,   "ublimage",   "Davinci UBL image",},
	{	-1,		    "",		  "",			},
};

static const table_entry_t uimage_comp[] = {
	{	IH_COMP_NONE,	"none",		"uncompressed",		},
	{	IH_COMP_BZIP2,	"bzip2",	"bzip2 compressed",	},
	{	IH_COMP_GZIP,	"gzip",		"gzip compressed",	},
	{	IH_COMP_LZMA,	"lzma",		"lzma compressed",	},
	{	IH_COMP_HARDLZMA, "jz_lzma",	"jz_lzma compressed",  },
	{	IH_COMP_LZO,	"lzo",		"lzo compressed",	},
	{	-1,		"",		"",			},
};

/*****************************************************************************/
/* Legacy format routines */
/*****************************************************************************/
int image_check_hcrc(const image_header_t *hdr)
{
	ulong hcrc;
	ulong len = image_get_header_size();
	image_header_t header;

	/* Copy header so we can blank CRC field for re-calculation */
	memmove(&header, (char *)hdr, image_get_header_size());
	image_set_hcrc(&header, 0);

	hcrc = crc32(0, (unsigned char *)&header, len);

	return (hcrc == image_get_hcrc(hdr));
}

int image_check_dcrc(const image_header_t *hdr)
{
	ulong data = image_get_data(hdr);
	ulong len = image_get_data_size(hdr);
	ulong dcrc = crc32_wd(0, (unsigned char *)data, len, CHUNKSZ_CRC32);

	return (dcrc == image_get_dcrc(hdr));
}

/**
 * image_multi_count - get component (sub-image) count
 * @hdr: pointer to the header of the multi component image
 *
 * image_multi_count() returns number of components in a multi
 * component image.
 *
 * Note: no checking of the image type is done, caller must pass
 * a valid multi component image.
 *
 * returns:
 *     number of components
 */
ulong image_multi_count(const image_header_t *hdr)
{
	ulong i, count = 0;
	uint32_t *size;

	/* get start of the image payload, which in case of multi
	 * component images that points to a table of component sizes */
	size = (uint32_t *)image_get_data(hdr);

	/* count non empty slots */
	for (i = 0; size[i]; ++i)
		count++;

	return count;
}

/**
 * image_multi_getimg - get component data address and size
 * @hdr: pointer to the header of the multi component image
 * @idx: index of the requested component
 * @data: pointer to a ulong variable, will hold component data address
 * @len: pointer to a ulong variable, will hold component size
 *
 * image_multi_getimg() returns size and data address for the requested
 * component in a multi component image.
 *
 * Note: no checking of the image type is done, caller must pass
 * a valid multi component image.
 *
 * returns:
 *     data address and size of the component, if idx is valid
 *     0 in data and len, if idx is out of range
 */
void image_multi_getimg(const image_header_t *hdr, ulong idx,
			ulong *data, ulong *len)
{
	int i;
	uint32_t *size;
	ulong offset, count, img_data;

	/* get number of component */
	count = image_multi_count(hdr);

	/* get start of the image payload, which in case of multi
	 * component images that points to a table of component sizes */
	size = (uint32_t *)image_get_data(hdr);

	/* get address of the proper component data start, which means
	 * skipping sizes table (add 1 for last, null entry) */
	img_data = image_get_data(hdr) + (count + 1) * sizeof(uint32_t);

	if (idx < count) {
		*len = uimage_to_cpu(size[idx]);
		offset = 0;

		/* go over all indices preceding requested component idx */
		for (i = 0; i < idx; i++) {
			/* add up i-th component size, rounding up to 4 bytes */
			offset += (uimage_to_cpu(size[i]) + 3) & ~3 ;
		}

		/* calculate idx-th component data address */
		*data = img_data + offset;
	} else {
		*len = 0;
		*data = 0;
	}
}

static void image_print_type(const image_header_t *hdr)
{
	const char *os, *arch, *type, *comp;

	os = genimg_get_os_name(image_get_os(hdr));
	arch = genimg_get_arch_name(image_get_arch(hdr));
	type = genimg_get_type_name(image_get_type(hdr));
	comp = genimg_get_comp_name(image_get_comp(hdr));

	printf("%s %s %s (%s)\n", arch, os, type, comp);
}

/**
 * image_print_contents - prints out the contents of the legacy format image
 * @ptr: pointer to the legacy format image header
 * @p: pointer to prefix string
 *
 * image_print_contents() formats a multi line legacy image contents description.
 * The routine prints out all header fields followed by the size/offset data
 * for MULTI/SCRIPT images.
 *
 * returns:
 *     no returned results
 */
void image_print_contents(const void *ptr)
{
	const image_header_t *hdr = (const image_header_t *)ptr;
	const char *p;

	p = IMAGE_INDENT_STRING;
	printf("%sImage Name:   %.*s\n", p, IH_NMLEN, image_get_name(hdr));
	if (IMAGE_ENABLE_TIMESTAMP) {
		printf("%sCreated:      ", p);
		genimg_print_time((time_t)image_get_time(hdr));
	}
	printf("%sImage Type:   ", p);
	image_print_type(hdr);
	printf("%sData Size:    ", p);
	genimg_print_size(image_get_data_size(hdr));
	printf("%sLoad Address: %08x\n", p, image_get_load(hdr));
	printf("%sEntry Point:  %08x\n", p, image_get_ep(hdr));

	if (image_check_type(hdr, IH_TYPE_MULTI) ||
			image_check_type(hdr, IH_TYPE_SCRIPT)) {
		int i;
		ulong data, len;
		ulong count = image_multi_count(hdr);

		printf("%sContents:\n", p);
		for (i = 0; i < count; i++) {
			image_multi_getimg(hdr, i, &data, &len);

			printf("%s   Image %d: ", p, i);
			genimg_print_size(len);

			if (image_check_type(hdr, IH_TYPE_SCRIPT) && i > 0) {
				/*
				 * the user may need to know offsets
				 * if planning to do something with
				 * multiple files
				 */
				printf("%s    Offset = 0x%08lx\n", p, data);
			}
		}
	}
}


#ifndef USE_HOSTCC
/**
 * image_get_ramdisk - get and verify ramdisk image
 * @rd_addr: ramdisk image start address
 * @arch: expected ramdisk architecture
 * @verify: checksum verification flag
 *
 * image_get_ramdisk() returns a pointer to the verified ramdisk image
 * header. Routine receives image start address and expected architecture
 * flag. Verification done covers data and header integrity and os/type/arch
 * fields checking.
 *
 * If dataflash support is enabled routine checks for dataflash addresses
 * and handles required dataflash reads.
 *
 * returns:
 *     pointer to a ramdisk image header, if image was found and valid
 *     otherwise, return NULL
 */
static const image_header_t *image_get_ramdisk(ulong rd_addr, uint8_t arch,
						int verify)
{
	const image_header_t *rd_hdr = (const image_header_t *)rd_addr;

	if (!image_check_magic(rd_hdr)) {
		puts("Bad Magic Number\n");
		bootstage_error(BOOTSTAGE_ID_RD_MAGIC);
		return NULL;
	}

	if (!image_check_hcrc(rd_hdr)) {
		puts("Bad Header Checksum\n");
		bootstage_error(BOOTSTAGE_ID_RD_HDR_CHECKSUM);
		return NULL;
	}

	bootstage_mark(BOOTSTAGE_ID_RD_MAGIC);
	image_print_contents(rd_hdr);

	if (verify) {
		puts("   Verifying Checksum ... ");
		if (!image_check_dcrc(rd_hdr)) {
			puts("Bad Data CRC\n");
			bootstage_error(BOOTSTAGE_ID_RD_CHECKSUM);
			return NULL;
		}
		puts("OK\n");
	}

	bootstage_mark(BOOTSTAGE_ID_RD_HDR_CHECKSUM);

	if (!image_check_os(rd_hdr, IH_OS_LINUX) ||
	    !image_check_arch(rd_hdr, arch) ||
	    !image_check_type(rd_hdr, IH_TYPE_RAMDISK)) {
		printf("No Linux %s Ramdisk Image\n",
				genimg_get_arch_name(arch));
		bootstage_error(BOOTSTAGE_ID_RAMDISK);
		return NULL;
	}

	return rd_hdr;
}
#endif /* !USE_HOSTCC */

/*****************************************************************************/
/* Shared dual-format routines */
/*****************************************************************************/
#ifndef USE_HOSTCC
ulong load_addr = CONFIG_SYS_LOAD_ADDR;	/* Default Load Address */
ulong save_addr;			/* Default Save Address */
ulong save_size;			/* Default Save Size (in bytes) */

static int on_loadaddr(const char *name, const char *value, enum env_op op,
	int flags)
{
	switch (op) {
	case env_op_create:
	case env_op_overwrite:
		load_addr = simple_strtoul(value, NULL, 16);
		break;
	default:
		break;
	}

	return 0;
}
U_BOOT_ENV_CALLBACK(loadaddr, on_loadaddr);

ulong getenv_bootm_low(void)
{
	char *s = getenv("bootm_low");
	if (s) {
		ulong tmp = simple_strtoul(s, NULL, 16);
		return tmp;
	}

#if defined(CONFIG_SYS_SDRAM_BASE)
	return CONFIG_SYS_SDRAM_BASE;
#elif defined(CONFIG_ARM)
	return gd->bd->bi_dram[0].start;
#else
	return 0;
#endif
}

phys_size_t getenv_bootm_size(void)
{
	phys_size_t tmp;
	char *s = getenv("bootm_size");
	if (s) {
		tmp = (phys_size_t)simple_strtoull(s, NULL, 16);
		return tmp;
	}
	s = getenv("bootm_low");
	if (s)
		tmp = (phys_size_t)simple_strtoull(s, NULL, 16);
	else
		tmp = 0;


#if defined(CONFIG_ARM)
	return gd->bd->bi_dram[0].size - tmp;
#else
	return gd->bd->bi_memsize - tmp;
#endif
}

phys_size_t getenv_bootm_mapsize(void)
{
	phys_size_t tmp;
	char *s = getenv("bootm_mapsize");
	if (s) {
		tmp = (phys_size_t)simple_strtoull(s, NULL, 16);
		return tmp;
	}

#if defined(CONFIG_SYS_BOOTMAPSZ)
	return CONFIG_SYS_BOOTMAPSZ;
#else
	return getenv_bootm_size();
#endif
}

void memmove_wd(void *to, void *from, size_t len, ulong chunksz)
{
	if (to == from)
		return;

#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
	while (len > 0) {
		size_t tail = (len > chunksz) ? chunksz : len;
		WATCHDOG_RESET();
		memmove(to, from, tail);
		to += tail;
		from += tail;
		len -= tail;
	}
#else	/* !(CONFIG_HW_WATCHDOG || CONFIG_WATCHDOG) */
	memmove(to, from, len);
#endif	/* CONFIG_HW_WATCHDOG || CONFIG_WATCHDOG */
}
#endif /* !USE_HOSTCC */

void genimg_print_size(uint32_t size)
{
#ifndef USE_HOSTCC
	printf("%d Bytes = ", size);
	print_size(size, "\n");
#else
	printf("%d Bytes = %.2f kB = %.2f MB\n",
			size, (double)size / 1.024e3,
			(double)size / 1.048576e6);
#endif
}

#if IMAGE_ENABLE_TIMESTAMP
void genimg_print_time(time_t timestamp)
{
#ifndef USE_HOSTCC
	struct rtc_time tm;

	to_tm(timestamp, &tm);
	printf("%4d-%02d-%02d  %2d:%02d:%02d UTC\n",
			tm.tm_year, tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
#else
	printf("%s", ctime(&timestamp));
#endif
}
#endif

/**
 * get_table_entry_name - translate entry id to long name
 * @table: pointer to a translation table for entries of a specific type
 * @msg: message to be returned when translation fails
 * @id: entry id to be translated
 *
 * get_table_entry_name() will go over translation table trying to find
 * entry that matches given id. If matching entry is found, its long
 * name is returned to the caller.
 *
 * returns:
 *     long entry name if translation succeeds
 *     msg otherwise
 */
char *get_table_entry_name(const table_entry_t *table, char *msg, int id)
{
	for (; table->id >= 0; ++table) {
		if (table->id == id)
#if defined(USE_HOSTCC) || !defined(CONFIG_NEEDS_MANUAL_RELOC)
			return table->lname;
#else
			return table->lname + gd->reloc_off;
#endif
	}
	return (msg);
}

const char *genimg_get_os_name(uint8_t os)
{
	return (get_table_entry_name(uimage_os, "Unknown OS", os));
}

const char *genimg_get_arch_name(uint8_t arch)
{
	return (get_table_entry_name(uimage_arch, "Unknown Architecture",
					arch));
}

const char *genimg_get_type_name(uint8_t type)
{
	return (get_table_entry_name(uimage_type, "Unknown Image", type));
}

const char *genimg_get_comp_name(uint8_t comp)
{
	return (get_table_entry_name(uimage_comp, "Unknown Compression",
					comp));
}

/**
 * get_table_entry_id - translate short entry name to id
 * @table: pointer to a translation table for entries of a specific type
 * @table_name: to be used in case of error
 * @name: entry short name to be translated
 *
 * get_table_entry_id() will go over translation table trying to find
 * entry that matches given short name. If matching entry is found,
 * its id returned to the caller.
 *
 * returns:
 *     entry id if translation succeeds
 *     -1 otherwise
 */
int get_table_entry_id(const table_entry_t *table,
		const char *table_name, const char *name)
{
	const table_entry_t *t;
#ifdef USE_HOSTCC
	int first = 1;

	for (t = table; t->id >= 0; ++t) {
		if (t->sname && strcasecmp(t->sname, name) == 0)
			return(t->id);
	}

	fprintf(stderr, "\nInvalid %s Type - valid names are", table_name);
	for (t = table; t->id >= 0; ++t) {
		if (t->sname == NULL)
			continue;
		fprintf(stderr, "%c %s", (first) ? ':' : ',', t->sname);
		first = 0;
	}
	fprintf(stderr, "\n");
#else
	for (t = table; t->id >= 0; ++t) {
#ifdef CONFIG_NEEDS_MANUAL_RELOC
		if (t->sname && strcmp(t->sname + gd->reloc_off, name) == 0)
#else
		if (t->sname && strcmp(t->sname, name) == 0)
#endif
			return (t->id);
	}
	debug("Invalid %s Type: %s\n", table_name, name);
#endif /* USE_HOSTCC */
	return (-1);
}

int genimg_get_os_id(const char *name)
{
	return (get_table_entry_id(uimage_os, "OS", name));
}

int genimg_get_arch_id(const char *name)
{
	return (get_table_entry_id(uimage_arch, "CPU", name));
}

int genimg_get_type_id(const char *name)
{
	return (get_table_entry_id(uimage_type, "Image", name));
}

int genimg_get_comp_id(const char *name)
{
	return (get_table_entry_id(uimage_comp, "Compression", name));
}

#ifndef USE_HOSTCC
/**
 * genimg_get_format - get image format type
 * @img_addr: image start address
 *
 * genimg_get_format() checks whether provided address points to a valid
 * legacy or FIT image.
 *
 * New uImage format and FDT blob are based on a libfdt. FDT blob
 * may be passed directly or embedded in a FIT image. In both situations
 * genimg_get_format() must be able to dectect libfdt header.
 *
 * returns:
 *     image format type or IMAGE_FORMAT_INVALID if no image is present
 */
int genimg_get_format(const void *img_addr)
{
	ulong format = IMAGE_FORMAT_INVALID;
	const image_header_t *hdr;
#if defined(CONFIG_FIT) || defined(CONFIG_OF_LIBFDT)
	char *fit_hdr;
#endif

	hdr = (const image_header_t *)img_addr;
	if (image_check_magic(hdr))
		format = IMAGE_FORMAT_LEGACY;
#if defined(CONFIG_FIT) || defined(CONFIG_OF_LIBFDT)
	else {
		fit_hdr = (char *)img_addr;
		if (fdt_check_header(fit_hdr) == 0)
			format = IMAGE_FORMAT_FIT;
	}
#endif

	return format;
}

/**
 * genimg_get_image - get image from special storage (if necessary)
 * @img_addr: image start address
 *
 * genimg_get_image() checks if provided image start adddress is located
 * in a dataflash storage. If so, image is moved to a system RAM memory.
 *
 * returns:
 *     image start address after possible relocation from special storage
 */
ulong genimg_get_image(ulong img_addr)
{
	ulong ram_addr = img_addr;

#ifdef CONFIG_HAS_DATAFLASH
	ulong h_size, d_size;

	if (addr_dataflash(img_addr)) {
		void *buf;

		/* ger RAM address */
		ram_addr = CONFIG_SYS_LOAD_ADDR;

		/* get header size */
		h_size = image_get_header_size();
#if defined(CONFIG_FIT)
		if (sizeof(struct fdt_header) > h_size)
			h_size = sizeof(struct fdt_header);
#endif

		/* read in header */
		debug("   Reading image header from dataflash address "
			"%08lx to RAM address %08lx\n", img_addr, ram_addr);

		buf = map_sysmem(ram_addr, 0);
		read_dataflash(img_addr, h_size, buf);

		/* get data size */
		switch (genimg_get_format(buf)) {
		case IMAGE_FORMAT_LEGACY:
			d_size = image_get_data_size(buf);
			debug("   Legacy format image found at 0x%08lx, "
					"size 0x%08lx\n",
					ram_addr, d_size);
			break;
#if defined(CONFIG_FIT)
		case IMAGE_FORMAT_FIT:
			d_size = fit_get_size(buf) - h_size;
			debug("   FIT/FDT format image found at 0x%08lx, "
					"size 0x%08lx\n",
					ram_addr, d_size);
			break;
#endif
		default:
			printf("   No valid image found at 0x%08lx\n",
				img_addr);
			return ram_addr;
		}

		/* read in image data */
		debug("   Reading image remaining data from dataflash address "
			"%08lx to RAM address %08lx\n", img_addr + h_size,
			ram_addr + h_size);

		read_dataflash(img_addr + h_size, d_size,
				(char *)(buf + h_size));

	}
#endif /* CONFIG_HAS_DATAFLASH */

	return ram_addr;
}

/**
 * fit_has_config - check if there is a valid FIT configuration
 * @images: pointer to the bootm command headers structure
 *
 * fit_has_config() checks if there is a FIT configuration in use
 * (if FTI support is present).
 *
 * returns:
 *     0, no FIT support or no configuration found
 *     1, configuration found
 */
int genimg_has_config(bootm_headers_t *images)
{
#if defined(CONFIG_FIT)
	if (images->fit_uname_cfg)
		return 1;
#endif
	return 0;
}

/**
 * boot_get_ramdisk - main ramdisk handling routine
 * @argc: command argument count
 * @argv: command argument list
 * @images: pointer to the bootm images structure
 * @arch: expected ramdisk architecture
 * @rd_start: pointer to a ulong variable, will hold ramdisk start address
 * @rd_end: pointer to a ulong variable, will hold ramdisk end
 *
 * boot_get_ramdisk() is responsible for finding a valid ramdisk image.
 * Curently supported are the following ramdisk sources:
 *      - multicomponent kernel/ramdisk image,
 *      - commandline provided address of decicated ramdisk image.
 *
 * returns:
 *     0, if ramdisk image was found and valid, or skiped
 *     rd_start and rd_end are set to ramdisk start/end addresses if
 *     ramdisk image is found and valid
 *
 *     1, if ramdisk image is found but corrupted, or invalid
 *     rd_start and rd_end are set to 0 if no ramdisk exists
 */
int boot_get_ramdisk(int argc, char * const argv[], bootm_headers_t *images,
		uint8_t arch, ulong *rd_start, ulong *rd_end)
{
	ulong rd_addr, rd_load;
	ulong rd_data, rd_len;
	const image_header_t *rd_hdr;
	void *buf;
#ifdef CONFIG_SUPPORT_RAW_INITRD
	char *end;
#endif
#if defined(CONFIG_FIT)
	const char	*fit_uname_config = images->fit_uname_cfg;
	const char	*fit_uname_ramdisk = NULL;
	ulong		default_addr;
	int		rd_noffset;
#endif
	const char *select = NULL;

	*rd_start = 0;
	*rd_end = 0;

	if (argc >= 2)
		select = argv[1];
	/*
	 * Look for a '-' which indicates to ignore the
	 * ramdisk argument
	 */
	if (select && strcmp(select, "-") ==  0) {
		debug("## Skipping init Ramdisk\n");
		rd_len = rd_data = 0;
	} else if (select || genimg_has_config(images)) {
#if defined(CONFIG_FIT)
		if (select) {
			/*
			 * If the init ramdisk comes from the FIT image and
			 * the FIT image address is omitted in the command
			 * line argument, try to use os FIT image address or
			 * default load address.
			 */
			if (images->fit_uname_os)
				default_addr = (ulong)images->fit_hdr_os;
			else
				default_addr = load_addr;

			if (fit_parse_conf(select, default_addr,
					   &rd_addr, &fit_uname_config)) {
				debug("*  ramdisk: config '%s' from image at "
						"0x%08lx\n",
						fit_uname_config, rd_addr);
			} else if (fit_parse_subimage(select, default_addr,
						&rd_addr, &fit_uname_ramdisk)) {
				debug("*  ramdisk: subimage '%s' from image at "
						"0x%08lx\n",
						fit_uname_ramdisk, rd_addr);
			} else
#endif
			{
				rd_addr = simple_strtoul(select, NULL, 16);
				debug("*  ramdisk: cmdline image address = "
						"0x%08lx\n",
						rd_addr);
			}
#if defined(CONFIG_FIT)
		} else {
			/* use FIT configuration provided in first bootm
			 * command argument. If the property is not defined,
			 * quit silently.
			 */
			rd_addr = map_to_sysmem(images->fit_hdr_os);
			rd_noffset = fit_get_node_from_config(images,
					FIT_RAMDISK_PROP, rd_addr);
			if (rd_noffset == -ENOLINK)
				return 0;
			else if (rd_noffset < 0)
				return 1;
		}
#endif

		/* copy from dataflash if needed */
		rd_addr = genimg_get_image(rd_addr);

		/*
		 * Check if there is an initrd image at the
		 * address provided in the second bootm argument
		 * check image type, for FIT images get FIT node.
		 */
		buf = map_sysmem(rd_addr, 0);
		switch (genimg_get_format(buf)) {
		case IMAGE_FORMAT_LEGACY:
			printf("## Loading init Ramdisk from Legacy "
					"Image at %08lx ...\n", rd_addr);

			bootstage_mark(BOOTSTAGE_ID_CHECK_RAMDISK);
			rd_hdr = image_get_ramdisk(rd_addr, arch,
							images->verify);

			if (rd_hdr == NULL)
				return 1;

			rd_data = image_get_data(rd_hdr);
			rd_len = image_get_data_size(rd_hdr);
			rd_load = image_get_load(rd_hdr);
			break;
#if defined(CONFIG_FIT)
		case IMAGE_FORMAT_FIT:
			rd_noffset = fit_image_load(images, FIT_RAMDISK_PROP,
					rd_addr, &fit_uname_ramdisk,
					&fit_uname_config, arch,
					IH_TYPE_RAMDISK,
					BOOTSTAGE_ID_FIT_RD_START,
					FIT_LOAD_IGNORED, &rd_data, &rd_len);
			if (rd_noffset < 0)
				return 1;

			images->fit_hdr_rd = map_sysmem(rd_addr, 0);
			images->fit_uname_rd = fit_uname_ramdisk;
			images->fit_noffset_rd = rd_noffset;
			break;
#endif
		default:
#ifdef CONFIG_SUPPORT_RAW_INITRD
			end = NULL;
			if (select)
				end = strchr(select, ':');
			if (end) {
				rd_len = simple_strtoul(++end, NULL, 16);
				rd_data = rd_addr;
			} else
#endif
			{
				puts("Wrong Ramdisk Image Format\n");
				rd_data = rd_len = rd_load = 0;
				return 1;
			}
		}
	} else if (images->legacy_hdr_valid &&
			image_check_type(&images->legacy_hdr_os_copy,
						IH_TYPE_MULTI)) {

		/*
		 * Now check if we have a legacy mult-component image,
		 * get second entry data start address and len.
		 */
		bootstage_mark(BOOTSTAGE_ID_RAMDISK);
		printf("## Loading init Ramdisk from multi component "
				"Legacy Image at %08lx ...\n",
				(ulong)images->legacy_hdr_os);

		image_multi_getimg(images->legacy_hdr_os, 1, &rd_data, &rd_len);
	} else {
		/*
		 * no initrd image
		 */
		bootstage_mark(BOOTSTAGE_ID_NO_RAMDISK);
		rd_len = rd_data = 0;
	}

	if (!rd_data) {
		debug("## No init Ramdisk\n");
	} else {
		*rd_start = rd_data;
		*rd_end = rd_data + rd_len;
	}
	debug("   ramdisk start = 0x%08lx, ramdisk end = 0x%08lx\n",
			*rd_start, *rd_end);

	return 0;
}

#ifdef CONFIG_SYS_BOOT_RAMDISK_HIGH
/**
 * boot_ramdisk_high - relocate init ramdisk
 * @lmb: pointer to lmb handle, will be used for memory mgmt
 * @rd_data: ramdisk data start address
 * @rd_len: ramdisk data length
 * @initrd_start: pointer to a ulong variable, will hold final init ramdisk
 *      start address (after possible relocation)
 * @initrd_end: pointer to a ulong variable, will hold final init ramdisk
 *      end address (after possible relocation)
 *
 * boot_ramdisk_high() takes a relocation hint from "initrd_high" environement
 * variable and if requested ramdisk data is moved to a specified location.
 *
 * Initrd_start and initrd_end are set to final (after relocation) ramdisk
 * start/end addresses if ramdisk image start and len were provided,
 * otherwise set initrd_start and initrd_end set to zeros.
 *
 * returns:
 *      0 - success
 *     -1 - failure
 */
int boot_ramdisk_high(struct lmb *lmb, ulong rd_data, ulong rd_len,
		  ulong *initrd_start, ulong *initrd_end)
{
	char	*s;
	ulong	initrd_high;
	int	initrd_copy_to_ram = 1;

	if ((s = getenv("initrd_high")) != NULL) {
		/* a value of "no" or a similar string will act like 0,
		 * turning the "load high" feature off. This is intentional.
		 */
		initrd_high = simple_strtoul(s, NULL, 16);
		if (initrd_high == ~0)
			initrd_copy_to_ram = 0;
	} else {
		/* not set, no restrictions to load high */
		initrd_high = ~0;
	}


#ifdef CONFIG_LOGBUFFER
	/* Prevent initrd from overwriting logbuffer */
	lmb_reserve(lmb, logbuffer_base() - LOGBUFF_OVERHEAD, LOGBUFF_RESERVE);
#endif

	debug("## initrd_high = 0x%08lx, copy_to_ram = %d\n",
			initrd_high, initrd_copy_to_ram);

	if (rd_data) {
		if (!initrd_copy_to_ram) {	/* zero-copy ramdisk support */
			debug("   in-place initrd\n");
			*initrd_start = rd_data;
			*initrd_end = rd_data + rd_len;
			lmb_reserve(lmb, rd_data, rd_len);
		} else {
			if (initrd_high)
				*initrd_start = (ulong)lmb_alloc_base(lmb,
						rd_len, 0x1000, initrd_high);
			else
				*initrd_start = (ulong)lmb_alloc(lmb, rd_len,
								 0x1000);

			if (*initrd_start == 0) {
				puts("ramdisk - allocation error\n");
				goto error;
			}
			bootstage_mark(BOOTSTAGE_ID_COPY_RAMDISK);

			*initrd_end = *initrd_start + rd_len;
			printf("   Loading Ramdisk to %08lx, end %08lx ... ",
					*initrd_start, *initrd_end);

			memmove_wd((void *)*initrd_start,
					(void *)rd_data, rd_len, CHUNKSZ);

#ifdef CONFIG_MP
			/*
			 * Ensure the image is flushed to memory to handle
			 * AMP boot scenarios in which we might not be
			 * HW cache coherent
			 */
			flush_cache((unsigned long)*initrd_start, rd_len);
#endif
			puts("OK\n");
		}
	} else {
		*initrd_start = 0;
		*initrd_end = 0;
	}
	debug("   ramdisk load start = 0x%08lx, ramdisk load end = 0x%08lx\n",
			*initrd_start, *initrd_end);

	return 0;

error:
	return -1;
}
#endif /* CONFIG_SYS_BOOT_RAMDISK_HIGH */

#ifdef CONFIG_SYS_BOOT_GET_CMDLINE
/**
 * boot_get_cmdline - allocate and initialize kernel cmdline
 * @lmb: pointer to lmb handle, will be used for memory mgmt
 * @cmd_start: pointer to a ulong variable, will hold cmdline start
 * @cmd_end: pointer to a ulong variable, will hold cmdline end
 *
 * boot_get_cmdline() allocates space for kernel command line below
 * BOOTMAPSZ + getenv_bootm_low() address. If "bootargs" U-boot environemnt
 * variable is present its contents is copied to allocated kernel
 * command line.
 *
 * returns:
 *      0 - success
 *     -1 - failure
 */
int boot_get_cmdline(struct lmb *lmb, ulong *cmd_start, ulong *cmd_end)
{
	char *cmdline;
	char *s;

	cmdline = (char *)(ulong)lmb_alloc_base(lmb, CONFIG_SYS_BARGSIZE, 0xf,
				getenv_bootm_mapsize() + getenv_bootm_low());

	if (cmdline == NULL)
		return -1;

	if ((s = getenv("bootargs")) == NULL)
		s = "";

	strcpy(cmdline, s);

	*cmd_start = (ulong) & cmdline[0];
	*cmd_end = *cmd_start + strlen(cmdline);

	debug("## cmdline at 0x%08lx ... 0x%08lx\n", *cmd_start, *cmd_end);

	return 0;
}
#endif /* CONFIG_SYS_BOOT_GET_CMDLINE */

#ifdef CONFIG_SYS_BOOT_GET_KBD
/**
 * boot_get_kbd - allocate and initialize kernel copy of board info
 * @lmb: pointer to lmb handle, will be used for memory mgmt
 * @kbd: double pointer to board info data
 *
 * boot_get_kbd() allocates space for kernel copy of board info data below
 * BOOTMAPSZ + getenv_bootm_low() address and kernel board info is initialized
 * with the current u-boot board info data.
 *
 * returns:
 *      0 - success
 *     -1 - failure
 */
int boot_get_kbd(struct lmb *lmb, bd_t **kbd)
{
	*kbd = (bd_t *)(ulong)lmb_alloc_base(lmb, sizeof(bd_t), 0xf,
				getenv_bootm_mapsize() + getenv_bootm_low());
	if (*kbd == NULL)
		return -1;

	**kbd = *(gd->bd);

	debug("## kernel board info at 0x%08lx\n", (ulong)*kbd);

#if defined(DEBUG) && defined(CONFIG_CMD_BDI)
	do_bdinfo(NULL, 0, 0, NULL);
#endif

	return 0;
}
#endif /* CONFIG_SYS_BOOT_GET_KBD */

#ifdef CONFIG_LMB
int image_setup_linux(bootm_headers_t *images)
{
	ulong of_size = images->ft_len;
	char **of_flat_tree = &images->ft_addr;
	ulong *initrd_start = &images->initrd_start;
	ulong *initrd_end = &images->initrd_end;
	struct lmb *lmb = &images->lmb;
	ulong rd_len;
	int ret;

	if (IMAGE_ENABLE_OF_LIBFDT)
		boot_fdt_add_mem_rsv_regions(lmb, *of_flat_tree);

	if (IMAGE_BOOT_GET_CMDLINE) {
		ret = boot_get_cmdline(lmb, &images->cmdline_start,
				&images->cmdline_end);
		if (ret) {
			puts("ERROR with allocation of cmdline\n");
			return ret;
		}
	}
	if (IMAGE_ENABLE_RAMDISK_HIGH) {
		rd_len = images->rd_end - images->rd_start;
		ret = boot_ramdisk_high(lmb, images->rd_start, rd_len,
				initrd_start, initrd_end);
		if (ret)
			return ret;
	}

	if (IMAGE_ENABLE_OF_LIBFDT) {
		ret = boot_relocate_fdt(lmb, of_flat_tree, &of_size);
		if (ret)
			return ret;
	}

	if (IMAGE_ENABLE_OF_LIBFDT && of_size) {
		ret = image_setup_libfdt(images, *of_flat_tree, of_size, lmb);
		if (ret)
			return ret;
	}

	return 0;
}
#endif /* CONFIG_LMB */
#endif /* !USE_HOSTCC */
