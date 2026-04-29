#include <common.h>
#include <bootargs_mem.h>
#include <asm/errno.h>

DECLARE_GLOBAL_DATA_PTR;

static int bootargs_has_mem(const char *args)
{
	const char *p = args;

	if (!args)
		return 0;

	while ((p = strstr(p, "mem=")) != NULL) {
		if (p == args || p[-1] == ' ')
			return 1;
		p += 4;
	}

	return 0;
}

static const char *bootargs_mem_select_by_size(unsigned long ram_size_mb)
{
	switch (ram_size_mb) {
#ifdef CONFIG_BOOTARGS_MEM_8M
	case 8:
		return CONFIG_BOOTARGS_MEM_8M;
#endif
#ifdef CONFIG_BOOTARGS_MEM_16M
	case 16:
		return CONFIG_BOOTARGS_MEM_16M;
#endif
#ifdef CONFIG_BOOTARGS_MEM_32M
	case 32:
		return CONFIG_BOOTARGS_MEM_32M;
#endif
#ifdef CONFIG_BOOTARGS_MEM_64M
	case 64:
		return CONFIG_BOOTARGS_MEM_64M;
#endif
#ifdef CONFIG_BOOTARGS_MEM_128M
	case 128:
		return CONFIG_BOOTARGS_MEM_128M;
#endif
#ifdef CONFIG_BOOTARGS_MEM_256M
	case 256:
		return CONFIG_BOOTARGS_MEM_256M;
#endif
#ifdef CONFIG_BOOTARGS_MEM_512M
	case 512:
		return CONFIG_BOOTARGS_MEM_512M;
#endif
	default:
		return NULL;
	}
}

const char *bootargs_mem_select(void)
{
	unsigned long ram_size_mb = 0;

	if (gd)
		ram_size_mb = gd->ram_size >> 20;

	return bootargs_mem_select_by_size(ram_size_mb);
}

int bootargs_mem_append(char *buf, unsigned int buf_size, const char *base)
{
	const char *mem_args;
	int len;

	if (!buf || !buf_size)
		return -EINVAL;

	if (!base)
		base = "";

	len = snprintf(buf, buf_size, "%s", base);
	if (len < 0 || len >= buf_size)
		return -ENOSPC;

	if (bootargs_has_mem(base))
		return len;

	mem_args = bootargs_mem_select();
	if (!mem_args || !mem_args[0])
		return len;

	while (len > 0 && buf[len - 1] == ' ')
		len--;

	if (len > 0) {
		if (len + 1 >= buf_size)
			return -ENOSPC;
		buf[len++] = ' ';
		buf[len] = '\0';
	}

	{
		int append_len = snprintf(buf + len, buf_size - len, "%s",
					  mem_args);
		if (append_len < 0 || append_len >= buf_size - len)
			return -ENOSPC;
		len += append_len;
	}

	return len;
}
