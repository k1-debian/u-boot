#include <common.h>
#include <exports.h>
#include <malloc.h>
#include <linux/types.h>
#include <linux/string.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/errno.h>
#include <asm/arch/base.h>
#include <asm/arch/clk.h>
#include <efuse.h>

//#define EFU_NO_REG_OPS

#ifdef EFU_NO_REG_OPS
#undef writel
#undef readl
#define writel(b, addr) {;}
#define readl(addr)	0
#endif

static int efuse_debug = 0;
static int efuse_en_gpio = -1;
static int efuse_en_active = 0;

#define WRITE_EFUSE 0
#define READ_EFUSE 1

#define EFUSE_CTRL		0x0
#define EFUSE_CFG		0x4
#define EFUSE_STATE		0x8
#define EFUSE_DATA(n)		(0xC + (n)*4)
#define CUSTID_PRT (1 << 15)
#define CHIP_PRT (1 << 14)

#define EFU_ROM_BASE    (0x200)

#define CHIP_ID_ADDR	(0x200)
#define CHIP_ID_END	(0x20F)
#define CHIP_ID_SIZE	(128)
#define RN_ADDR		(0x210)
#define RN_END		(0x21F)
#define RN_SIZE		(128)
#define CUT_ID_ADDR	(0x220)
#define CUT_ID_END	(0x22F)
#define CUT_ID_SIZE	(128)	/* X1000 CUT_ID(USER_ID) is 128 bit */
#define PRT_ADDR	(0x23E)
#define PRT_END	        (0x23F)
#define PRT_SIZE	(16)

static uint32_t seg_addr[] = {
	CHIP_ID_ADDR,
	RN_ADDR,
	CUT_ID_ADDR,
	PRT_ADDR,
};

static int get_segment_bounds(unsigned int start, unsigned int *seg_start, unsigned int *seg_end)
{
	if (start >= CHIP_ID_ADDR && start <= CHIP_ID_END) {
		*seg_start = CHIP_ID_ADDR;
		*seg_end = CHIP_ID_END;
	} else if (start >= RN_ADDR && start <= RN_END) {
		*seg_start = RN_ADDR;
		*seg_end = RN_END;
	} else if (start >= CUT_ID_ADDR && start <= CUT_ID_END) {
		*seg_start = CUT_ID_ADDR;
		*seg_end = CUT_ID_END;
	} else if (start >= PRT_ADDR && start <= PRT_END) {
		*seg_start = PRT_ADDR;
		*seg_end = PRT_END;
	} else {
		return -EINVAL;
	}

	return 0;
}

static inline unsigned int max_integral_multiple(unsigned int val, unsigned int base)
{
	unsigned int max_int;
	max_int = val / base;
	max_int += val % base ? 1 : 0;
	return max_int;
}
static uint32_t efuse_readl(uint32_t reg_off)
{
	return readl(EFUSE_BASE + reg_off);
}

static void efuse_writel(uint32_t val, uint32_t reg_off)
{
	writel(val, EFUSE_BASE + reg_off);
}

static int adjust_efuse(void)
{
	uint32_t val, ns;
	int i, rd_strobe, wr_strobe;
	uint32_t rd_adj, wr_adj;
	int h2clk = clk_get_rate(H2CLK);

	ns = 1000000000 / h2clk;
	printf("rate = %d, ns = %d\n", h2clk, ns);

	for(i = 0; i < 0x4; i++)
		if((( i + 1) * ns ) > 2)
			break;
	if(i == 0x4) {
		printf("get efuse cfg rd_adj fail!\n");
		return -EFAULT;
	}
	rd_adj = wr_adj = i;

	for(i = 0; i < 0x8; i++)
		if(((rd_adj + i + 3) * ns ) > 15)
			break;
	if(i == 0x8) {
		printf("get efuse cfg rd_strobe fail!\n");
		return -EFAULT;
	}
	rd_strobe = i;

	for(i = 0; i < 0x1f; i += 100) {
		val = (wr_adj + i + 916) * ns;
		if( val > 4 * 1000 && val < 6 *1000)
			break;
	}
	if(i >= 0x1f) {
		printf("get efuse cfg wd_strobe fail!\n");
		return -EFAULT;
	}
	wr_strobe = i;

	printf("rd_adj = %d | rd_strobe = %d | "
	       "wr_adj = %d | wr_strobe = %d\n", rd_adj, rd_strobe,
	       wr_adj, wr_strobe);
	/*set configer register*/
	val = rd_adj << 20 | rd_strobe << 16 | wr_adj << 12 | wr_strobe;
	efuse_writel(val, EFUSE_CFG);

	printf("h2clk is %d, efucfg_reg 0x%x\n",h2clk,efuse_readl(EFUSE_CFG));
	return 0;
}

static void boost_vddq(int gpio)
{
	int val;
	debug_cond(efuse_debug, "boost vddq\n");
	gpio_direction_output(gpio , efuse_en_active);
	do {
		val = gpio_get_value(gpio);
		debug_cond(efuse_debug, "gpio %d level %d\n", gpio, val);
	} while (val != efuse_en_active);
	mdelay(10);		/*  mdelay(10) wait for EFUSE VDDQ setup. */
}

static void reduce_vddq(int gpio)
{
	int val;
	debug_cond(efuse_debug, "reduce vddq\n");
	gpio_direction_output(gpio, !efuse_en_active);
	do {
		val = gpio_get_value(gpio);
		debug_cond(efuse_debug, "gpio %d level %d\n", gpio, val);
	} while (val == efuse_en_active);
	mdelay(10);		/*  mdelay(10) wait for EFUSE VDDQ fall down. */
}

static int efuse_read_data(void *buf, uint32_t start_addr, int length)
{
	int i = 0;
	char *tmp_buf = NULL;
	int32_t *ptmp_buf = NULL;
	char * pbuf =buf;
	uint32_t data_length = length - 1;
	uint32_t word_num,val;
	unsigned int addr = start_addr - EFU_ROM_BASE;

	debug_cond(efuse_debug, "efuse read length %d from offset 0x%x\n",length, start_addr);

	if (efuse_en_gpio >= 0)
		reduce_vddq(efuse_en_gpio);

	word_num = max_integral_multiple(length, 4);

	tmp_buf = malloc(word_num * sizeof(int32_t));
	if (!tmp_buf)
		return -ENOMEM;
	memset(tmp_buf , 0 , word_num * sizeof(int32_t));
	ptmp_buf = (int32_t *)tmp_buf;

	/* clear read done staus */
	efuse_writel(0, EFUSE_STATE);

	val = addr << 21 | data_length << 16;
	efuse_writel(val, EFUSE_CTRL);
	/* enable read */
	val = efuse_readl(EFUSE_CTRL);
	val |= 1;
	efuse_writel(val, EFUSE_CTRL);
	/* wait read done status */
	while(!(efuse_readl(EFUSE_STATE) & 1));

	for(i = 0; i < word_num; i ++) {
		val = efuse_readl(EFUSE_DATA(i));
		debug_cond(efuse_debug,"EFUDATA[0x%x]:0x%08x\n",(EFUSE_BASE+EFUSE_DATA(i)),
			   val);
		*(ptmp_buf + i) = val;
	}

	/* clear read done staus */
	efuse_writel(0, EFUSE_STATE);

	for (i = 0 ; i < length; i++) {
		pbuf[i] = tmp_buf[i];
		/* printf("0x%02x, 0x%02x\n", pbuf[i],tmp_buf[i]); */
	}
	if (efuse_debug) {
		int i = 0;
		printf("====read data infomation====\n");
		for (i = 0; i < word_num; i++) {
			printf("0x%03x:0x%08x\n", i, *((int32_t *)pbuf + i));
		}
		printf("============================\n");
	}

	free(tmp_buf);
	return word_num;
}

#define EFUSE_CHECK
#define EFUSE_W_TIMEOUT	(100*800)

/**
* @brief write data to efuse at offset, with data length.
*
* @param buf			stores hex value.
* @param start_addr
* @param length
*
* @return
*/
static int efuse_write_data(void *buf, uint32_t start_addr, int length)
{
	int i = 0, ret = 0;
	unsigned int * pbuf = buf;
	uint32_t data_length = length - 1;
	unsigned long long tmp_data;
	uint32_t word_num, val;
	unsigned int addr = start_addr - EFU_ROM_BASE;
	int timeout = EFUSE_W_TIMEOUT;		//vddq high is less than 1 sec

	debug_cond(efuse_debug, "write data to start_addr: %x, length: %d\n", start_addr, length);

	if  (efuse_en_gpio < 0) {
		error("efuse gpio is not init");
		return -ENODEV;
	}

	word_num = max_integral_multiple(length, 4);

	for (i = 0; i < word_num; i++) {
		debug_cond(efuse_debug, "0x%08x\n", pbuf[i]);
	}


	if(word_num > 8) {
		debug_cond(efuse_debug, "strongly recommend operate each segment separately\n");
	} else {
		for(i = 0; i < word_num; i++) {
			val = pbuf[i];
			debug_cond(efuse_debug,"====write data to register====\n");
			debug_cond(efuse_debug,"%d(0x%x):0x%x\n",i,(EFUSE_BASE+EFUSE_DATA(i)),val);
			debug_cond(efuse_debug, "%d(0x%x):0x%08x\n", i, (EFUSE_BASE + EFUSE_DATA(i)), val);
			efuse_writel(val, EFUSE_DATA(i));
		}
	}



	/*
	 * set write Programming address and data length
	 */
	val = 0;
	val = addr  << 21 | data_length << 16;
	efuse_writel(val, EFUSE_CTRL);

	/*
	 * Programming EFUSE enable
	 */
	//val = efuse_readl(EFUSE_CTRL);
	val |= 1 << 15;
	efuse_writel(val, EFUSE_CTRL);

	/* Connect VDDQ pin from 2.5V */
	boost_vddq(efuse_en_gpio);

	/* clear write done status */
	efuse_writel(0, EFUSE_STATE);

	/* enable write */
	//val = efuse_readl(EFUSE_CTRL);
	val |= 2;
	efuse_writel(val, EFUSE_CTRL);

	/* wait write done status */
	while(!(efuse_readl(EFUSE_STATE) & 0x2) &&  --timeout);

	/* Disconnect VDDQ pin from 2.5V. */
	reduce_vddq(efuse_en_gpio);
	/* clear PG_EN */
	efuse_writel(0, EFUSE_CTRL);
	/* clear write done status */
	efuse_writel(0, EFUSE_STATE);
	if(!timeout) {
		error("write efuse failed");
		ret = -EFAULT;
		goto out;
	}

out:
	return ret;
}

static int check_vaild_addr(int rw, unsigned int start, unsigned int length)
{
	unsigned int length_bits = length * 8;
	unsigned int val;

	if(rw == WRITE_EFUSE)
		printf("write segment ");
	else if(rw == READ_EFUSE)
		printf("read segment ");
	else
		return -EINVAL;

	switch(start) {
	case CHIP_ID_ADDR ... CHIP_ID_END:
		printf("chip id,");
		if(length_bits > CHIP_ID_SIZE) {
			printf("data max length %d bits ", CHIP_ID_SIZE);
			return -EINVAL;
		}
		if(rw == WRITE_EFUSE) {
			val = efuse_readl(EFUSE_STATE);
			if(val & CHIP_PRT) {
				printf("is protected\n");
				return -EINVAL;
			}
		}
		break;
	case RN_ADDR ... RN_END:
		printf("random number,");
		if(length_bits > RN_SIZE) {
			printf("data max length %d bits ", RN_SIZE);
			return -EINVAL;
		}
		if(rw == WRITE_EFUSE) {
			printf("only support read\n");
			return -EINVAL;
		}
		break;
	case CUT_ID_ADDR ... CUT_ID_END:
		printf("customer id,");
		if(length_bits > CUT_ID_SIZE) {
			printf("data max length %d bits ", CUT_ID_SIZE);
			return -EINVAL;
		}
		if(rw == WRITE_EFUSE) {
			val = efuse_readl(EFUSE_STATE);
			if(val & CUSTID_PRT) {
				printf("is protected\n");
				return -EINVAL;
			}
		}
		break;
	case PRT_ADDR ... PRT_END:
		printf("protect id,");
		if(length_bits > PRT_SIZE) {
			printf("data max length %d bits ", PRT_SIZE);
			return -EINVAL;
		}
		break;
	default:
		printf("segment id addr err!\n");
		return -EINVAL;
	}

	printf("addr %x length %d bits end %x\n", start, length_bits, start + length - 1);
	return 0;
}

static int fromhex(int a)
{
    if (a >= '0' && a <= '9')
        return a - '0';
    else if (a >= 'a' && a <= 'f')
        return a - 'a' + 10;
    else if (a >= 'A' && a <= 'F')
        return a - 'A' + 10;
    return 0;
}

static int hex2bin(const char *hex, char *bin, int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        if (hex[0] == 0 || hex[1] == 0)
        {
            return i;
        }
        *bin++ = fromhex (hex[0]) * 16 + fromhex (hex[1]);
        hex += 2;
    }
    return i;
}

/**
* @brief efuse write data to efuse.
*
* @param buf            !!, buf stores a hex string, like "E0", "AB", "12"
* @param length
* @param offset         !!, offset , which starts at 0
*
* @return
*/
int efuse_write(void *buf, int length, off_t offset)
{
	unsigned int start = (offset + EFU_ROM_BASE);
	unsigned int start_read = start;
	int ret = -EPERM;
	unsigned int data_length;
	unsigned long long write_val;
	unsigned int *data_buf = NULL;
	unsigned int *read_data = NULL;
	unsigned int *expect_data = NULL;
	unsigned char *read_bytes;
	unsigned char *write_bytes;
	unsigned char *expect_bytes;
	int word_num;
	int i;
	int input_length;
	int is_prt = (start >= PRT_ADDR && start <= PRT_END);
	char *pbuf = NULL;

	if (!buf || length <= 0)
		return -EINVAL;

	pbuf = malloc(length + 1);
	if (!pbuf)
		return -ENOMEM;
	memset(pbuf, 0, length + 1);
	memcpy(pbuf, buf, length);
	input_length = strnlen(pbuf, length);
	if (!input_length) {
		ret = -EINVAL;
		goto out;
	}

	write_val = simple_strtoull(pbuf, NULL, 16);
	if (is_prt) {
		if (write_val > 0xffffULL) {
			printf("protect id only supports 16-bit data\n");
			ret = -EINVAL;
			goto out;
		}
		data_length = PRT_SIZE / 8;
	} else {
		data_length = max_integral_multiple(input_length, 2);
	}

	word_num = max_integral_multiple(data_length, 4);

	debug_cond(efuse_debug, "length = %d\n", length);
	debug_cond(efuse_debug, "offset = %x\n", (unsigned int)offset);
	debug_cond(efuse_debug, "data_length = %d\n", data_length);

	if (check_vaild_addr(WRITE_EFUSE, start, data_length)) {
		ret = -EINVAL;
		goto out;
	}

	read_data = malloc(word_num * 4);
	data_buf = malloc(word_num * 4);
	expect_data = malloc(word_num * 4);
	if (!read_data || !data_buf || !expect_data) {
		printf("error allocate efuse buffers!\n");
		ret = -ENOMEM;
		goto out;
	}
	memset(read_data, 0, word_num * 4);
	memset(data_buf, 0, word_num * 4);
	memset(expect_data, 0, word_num * 4);

	*data_buf = write_val;
	for(i = 0; i < word_num; i++) {
		debug_cond(efuse_debug, "databuf[%d]: %x\n", i, data_buf[i]);
	}

	ret = efuse_read_data(read_data, start_read, data_length);
	if (ret < 0)
		goto out;
	ret = 0;

	read_bytes = (unsigned char *)read_data;
	write_bytes = (unsigned char *)data_buf;
	expect_bytes = (unsigned char *)expect_data;

	if (is_prt) {
		int need_write = 0;

		for (i = 0; i < data_length; i++) {
			expect_bytes[i] = read_bytes[i] | write_bytes[i];
			write_bytes[i] &= ~read_bytes[i];
			if (write_bytes[i])
				need_write = 1;
		}

		if (!need_write)
			goto verify;
	} else {
		for (i = 0; i < data_length; i++) {
			if (read_bytes[i]) {
				error("The position has been written data, write efuse failed!\n");
				ret = -EINVAL;
				goto out;
			}
		}
		memcpy(expect_data, data_buf, data_length);
	}

	if (is_prt || (start + data_length) <= 0x230) {
		ret = efuse_write_data(data_buf, start, data_length);
	} else if ((start + data_length) < CUT_ID_END) {
		unsigned char *pdata = (unsigned char *)data_buf;
		unsigned int data_left = data_length;
		unsigned int write_cnt = 0;
		int data_word_num;

		if(start < 0x230) {
			write_cnt = 0x230 - start;
			ret = efuse_write_data(pdata, start, 0x230 - start);
			if (ret)
				goto out;
			pdata += write_cnt;
			data_left -= write_cnt;
			start += write_cnt;
		}

		if(start % 4) {
			printf("start addr should be 4 byte align\n");
			ret = -EINVAL;
			goto out;
		}

		data_word_num = max_integral_multiple(data_left, 4);
		for(i = 0; i < data_word_num; i++) {
			write_cnt = data_left < 4 ? data_left : 4;
			ret = efuse_write_data(pdata, start, write_cnt);
			if (ret)
				goto out;
			data_left -= write_cnt;
			pdata += write_cnt;
			start += write_cnt;
		}
	}

#ifdef EFUSE_CHECK
verify:
	memset(read_data, 0, word_num * 4);
	ret = efuse_read_data(read_data, start_read, data_length);
	if (ret < 0)
		goto out;
	ret = 0;

	if (memcmp(read_data, expect_data, data_length)) {
		error(" compare error . write efuse failed");
		ret = -EFAULT;
	}
#endif

out:
	free(pbuf);
	free(read_data);
	free(data_buf);
	free(expect_data);
	return ret;
}

int efuse_read(void *buf, int length, off_t offset)
{
	unsigned int start = (offset + EFU_ROM_BASE);
	int ret = -EPERM;

	if (check_vaild_addr(READ_EFUSE, start, length))
		return -EINVAL;

	ret = efuse_read_data(buf, start, length);
	return ret;
}

int efuse_write_segment(void *buf, int length, off_t offset)
{
	unsigned int start = offset + EFU_ROM_BASE;
	unsigned int seg_start;
	unsigned int seg_end;
	unsigned int max_input_length;
	int input_length;

	if (!buf || length <= 0)
		return -EINVAL;

	if (get_segment_bounds(start, &seg_start, &seg_end))
		return -EINVAL;

	max_input_length = (seg_end - seg_start + 1 - (start - seg_start)) * 2;
	if (length > max_input_length)
		length = max_input_length;

	input_length = strnlen((char *)buf, length);
	if (!input_length)
		return -EINVAL;

	return efuse_write(buf, input_length, offset);
}

int efuse_read_segment(void *buf, int length, off_t offset)
{
	unsigned int start = offset + EFU_ROM_BASE;
	unsigned int seg_start;
	unsigned int seg_end;
	int ret;

	if (!buf || length <= 0)
		return -EINVAL;

	if (get_segment_bounds(start, &seg_start, &seg_end))
		return -EINVAL;

	if (length > (seg_end - seg_start + 1 - (start - seg_start)))
		length = seg_end - seg_start + 1 - (start - seg_start);

	ret = efuse_read(buf, length, offset);
	if (ret < 0)
		return ret;

	return length;
}

int efuse_read_chipid(void *buf, int length, off_t offset)
{
	int ret = -EPERM;

	ret = efuse_read(buf, length, offset);
	if(ret < 0) {
		printf("efuse_read_chipid: read chip id error\n");
		return ret;
	}

	return ret;
}

int efuse_read_id(void *buf, int length, int id)
{
	int ret = -EPERM;
	int offset = 0;
	id = id + 1;
	switch(id) {
		case EFUSE_R_CHIP_ID:
			offset = CHIP_ID_ADDR;
			length = CHIP_ID_SIZE / 8;
			break;
		case EFUSE_R_USER_ID:
			offset = CUT_ID_ADDR;
			length = CUT_ID_SIZE / 8;
			break;
		case EFUSE_R_RN:
			offset = RN_ADDR;
			length = RN_SIZE / 8;
			break;
		case EFUSE_R_PRT:
			offset = PRT_ADDR;
			length = PRT_SIZE / 8;
			break;
		default:
			printf("Unkown id !\n");
			return -EPERM;
	}

	offset = offset - EFU_ROM_BASE;

	ret = efuse_read(buf, length, offset);
	if(ret < 0) {
		printf("efuse_read_id: read id error\n");
		return ret;
	}

	return length;
}

int efuse_init(int gpio_pin, int active)
{
	if (gpio_pin >= 0) {
		if (efuse_en_gpio >= 0) gpio_free(efuse_en_gpio);
		efuse_en_gpio = gpio_request(gpio_pin, "VDDQ");
		if (efuse_en_gpio < 0) return efuse_en_gpio;
		efuse_en_active = active;
	} else {
		efuse_en_gpio = -1;
	}
	if(adjust_efuse() < 0)
		return -1;
	return 0;
}

void efuse_deinit(void)
{
	if (efuse_en_gpio >= 0)
		gpio_free(efuse_en_gpio);
	efuse_en_gpio = -1;
	return;
}

void efuse_debug_enable(int enable)
{
	efuse_debug = !!enable;
	return;
}
