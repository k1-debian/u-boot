#include <common.h>

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


static char* process_mem_bootargs(char *cmdargs, int ram_size)
{
	char *args_mem = NULL;
	char *args_mem_end = NULL;
	unsigned int mem_start;
	unsigned int rmem_start;

	args_mem = strstr(cmdargs, ARGS_MEM_RESERVED);

	args_mem_end = args_mem + strlen(ARGS_MEM_RESERVED);

	ram_size -= CONFIG_RMEM_MB;

	/* mem=xxxM@0x0*/
	mem_start = 0;
	args_mem += string_copy(args_mem, "mem=", 4);
	args_mem += int_to_string(args_mem, ram_size, 10);
	args_mem += string_copy(args_mem, "M@0x", 4);
	args_mem += int_to_string(args_mem, mem_start, 16);

	if (CONFIG_RMEM_MB) {
		args_mem += string_copy(args_mem, " ", 1);

		/* rmem=xxxM@0xxxx */
		rmem_start = ram_size * 1024 * 1024;
		args_mem += string_copy(args_mem, "rmem=", 5);
		args_mem += int_to_string(args_mem, CONFIG_RMEM_MB, 10);
		args_mem += string_copy(args_mem, "M@0x", 4);
		args_mem += int_to_string(args_mem, rmem_start, 16);
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