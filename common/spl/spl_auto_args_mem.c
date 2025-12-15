#include <common.h>

#define ALIGN_4K	4
#ifndef ALIGN
#define ALIGN(a,b)		(((a) + ((b) - 1)) & (~((b)-1)))
#endif

/* number to string */
static unsigned int int_to_string(char *str, unsigned int value, int base)
{
	int j = 19;
	int data, len;
	char buf[20];

	if (base == 10) {
		do {
			buf[j--] = (value % 10) + '0';
			value = value / 10;
		} while (value);
	}

	if (base == 16) {
		do {
			data = value % 16;

			if (data > 9)
				buf[j--] = 'a' + data - 10;
			else
				buf[j--] = '0' + data;

			value = value >> 4;
		} while (value);
	}

	len = 19 - j;

	memcpy(str, buf + j + 1, len);

	return len;
}

static unsigned int string_copy(char *dest, char *src, unsigned int size)
{
	memcpy(dest, src, size);

	return size;
}

static char *add_mem(char *str, char *tag,
	unsigned int start, unsigned int size, int kb_flags)
{
	if (size == 0)
		return str;
	if (start != 0) 
		str += string_copy(str, " ", 1);
	str += string_copy(str, tag, strlen(tag));
	str += int_to_string(str, size, 10);
	if (kb_flags) {
		start *= 1024;
		str += string_copy(str, "K@0x", 4);
	} else {
		start *= (1024 * 1024);
		str += string_copy(str, "M@0x", 4);
	}
	str += int_to_string(str, start, 16);

	return str;
}

static char* process_mem_bootargs(char *cmdargs, int ram_size)
{
	char *args_mem = NULL;
	char *args_mem_end = NULL;
	unsigned int rmem_size = 0;
	unsigned int rtos_size = 0;
	unsigned int lcd_mem_size = 0;
	unsigned int share_mem_size = 0;
	unsigned int nmem_size = 0;
	unsigned int vpu_mem_size = 0;
	unsigned int real_size = ram_size;
	int kb_flags = 0;

	args_mem = strstr(cmdargs, "[mem-start");

	args_mem_end = strstr(cmdargs, "mem-end]") + strlen("mem-end]");

	if (ram_size >= 256)
		ram_size = 256;

	if (CONFIG_RMEM_KB || CONFIG_NMEM_KB || CONFIG_RTOS_SIZE_KB || CONFIG_LCD_MEM_KB || CONFIG_SHARE_MEM_KB || CONFIG_VPU_MEM_KB)
		kb_flags = 1;

	if (kb_flags) {
		rmem_size = CONFIG_RMEM_MB ? (CONFIG_RMEM_MB * 1024) : CONFIG_RMEM_KB;
		rmem_size = ALIGN(rmem_size, ALIGN_4K);

		nmem_size = CONFIG_NMEM_MB ? (CONFIG_NMEM_MB * 1024) : CONFIG_NMEM_KB;
		nmem_size = ALIGN(nmem_size, ALIGN_4K);

		rtos_size = CONFIG_RTOS_SIZE_MB ? (CONFIG_RTOS_SIZE_MB * 1024) : CONFIG_RTOS_SIZE_KB;
		rtos_size = ALIGN(rtos_size, ALIGN_4K);

		lcd_mem_size = CONFIG_LCD_MEM_MB ? (CONFIG_LCD_MEM_MB * 1024) : CONFIG_LCD_MEM_KB;
		lcd_mem_size = ALIGN(lcd_mem_size, ALIGN_4K);

		share_mem_size = CONFIG_SHARE_MEM_MB ? (CONFIG_SHARE_MEM_MB * 1024) : CONFIG_SHARE_MEM_KB;
		share_mem_size = ALIGN(share_mem_size, ALIGN_4K);

		vpu_mem_size = CONFIG_VPU_MEM_MB ? (CONFIG_VPU_MEM_MB * 1024) : CONFIG_VPU_MEM_KB;
		vpu_mem_size = ALIGN(vpu_mem_size, ALIGN_4K);

		ram_size *= 1024;
	} else {
		rmem_size = CONFIG_RMEM_MB;
		nmem_size = CONFIG_NMEM_MB;
		rtos_size = CONFIG_RTOS_SIZE_MB;
		lcd_mem_size = CONFIG_LCD_MEM_MB;
		share_mem_size = CONFIG_SHARE_MEM_MB;
		vpu_mem_size = CONFIG_VPU_MEM_MB;
	}

	ram_size = ram_size - rmem_size - nmem_size - rtos_size - lcd_mem_size - share_mem_size - vpu_mem_size;

	args_mem = add_mem(args_mem, "mem=", 0, ram_size, kb_flags);
	args_mem = add_mem(args_mem, "vpu_mem=", ram_size, vpu_mem_size, kb_flags);
	ram_size += vpu_mem_size;
	args_mem = add_mem(args_mem, "rmem=", ram_size, rmem_size, kb_flags);
	ram_size += rmem_size;
	args_mem = add_mem(args_mem, "nmem=", ram_size, nmem_size, kb_flags);
	ram_size += nmem_size;
	args_mem = add_mem(args_mem, "rtos_size=", ram_size, rtos_size, kb_flags);
	ram_size += rtos_size;
	args_mem = add_mem(args_mem, "lcd_mem=", ram_size, lcd_mem_size, kb_flags);
	ram_size += lcd_mem_size;
	args_mem = add_mem(args_mem, "share_mem=", ram_size, share_mem_size, kb_flags);
	ram_size += share_mem_size;

	if (real_size > 256) {
		real_size = real_size - 256;
		real_size = kb_flags ? real_size * 1024 : real_size;
		args_mem = add_mem(args_mem, "mem=", (kb_flags ? (768 * 1024) : 768), real_size, kb_flags);
	}

	memmove(args_mem, args_mem_end, strlen(args_mem_end) + 1);

	return cmdargs;
}


extern unsigned int get_ddr_size(void);

char* spl_board_process_mem_bootargs(char *cmdargs)
{
	unsigned int ram_size = get_ddr_size();

	return process_mem_bootargs(cmdargs, ram_size);
}