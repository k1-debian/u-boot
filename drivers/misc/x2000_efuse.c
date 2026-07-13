#include <common.h>
#include <exports.h>
#include <malloc.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/err.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/errno.h>
#include <asm/arch/base.h>
#include <asm/arch/clk.h>
#include <asm/arch/efuse.h>
#include <efuse.h>
#include "hamming.h"

static int efuse_debug = 0;
static int efuse_gpio = -1;
static int efuse_en_active = 0;

static struct seg_info info;

// 从 src 的第 src_bit_offset 位开始，拷贝 num_bits 个比特
// 到 dst 的第 dst_bit_offset 位开始的位置
static void bit_copy(void *dst, size_t dst_bit_offset,
		const void *src, size_t src_bit_offset,
		size_t num_bits)
{
	if (num_bits == 0) return;

	uint8_t *d = (uint8_t *)dst;
	const uint8_t *s = (const uint8_t *)src;
	int i;

	for (i = 0; i < num_bits; i++) {
		// 计算源字节和位
		size_t src_byte = (src_bit_offset + i) / 8;
		size_t src_bit  = (src_bit_offset + i) % 8;

		// 读取源 bit（假设小端，bit 0 是最低位）
		uint8_t src_val = (s[src_byte] >> src_bit) & 1;

		// 计算目标字节和位
		size_t dst_byte = (dst_bit_offset + i) / 8;
		size_t dst_bit  = (dst_bit_offset + i) % 8;

		// 清除目标 bit，再写入
		d[dst_byte] &= ~(1U << dst_bit);          // 清 0
		d[dst_byte] |= (src_val << dst_bit);      // 写入
	}
}


static void efuse_segment_bytes_to_hex(const unsigned char *raw,
		unsigned int raw_bytes, char *hex)
{
	unsigned int i;

	for (i = 0; i < raw_bytes; i++)
		sprintf(hex + (i * 2), "%02x", raw[raw_bytes - 1 - i]);
	hex[raw_bytes * 2] = '\0';
}

static uint32_t efuse_readl(uint32_t reg_off)
{
	return readl(EFUSE_BASE + reg_off);
}

static void efuse_writel(uint32_t val, uint32_t reg_off)
{
	writel(val, EFUSE_BASE + reg_off);
}

static void boost_vddq(int gpio)
{
	int val;
	printf("boost vddq\n");
	if (gpio != -1) {
		gpio_direction_output(gpio, efuse_en_active);
		do {
			val = gpio_get_value(gpio);
			printf("gpio %d level %d\n",gpio,val);
		} while (val != efuse_en_active);
		mdelay(10);		/*  mdelay(10) wait for EFUSE VDDQ setup. */
	}
}

static void reduce_vddq(int gpio)
{
	int val;
	printf("reduce vddq\n");
	if (gpio != -1) {
		gpio_direction_output(gpio, !efuse_en_active);
		do {
			val = gpio_get_value(gpio);
			printf("gpio %d level %d\n",gpio,val);
		} while (val == efuse_en_active);
		mdelay(10);		/*  mdelay(10) wait for EFUSE VDDQ fall down. */
	}
}


static void otp_r(uint32_t addr, uint32_t wlen)
{
	unsigned int val;
	int n;

	efuse_writel(0, EFUSE_CTRL);

	for(n = 0; n < 8; n++)
		efuse_writel(0, EFUSE_DATA(n));

	/* set read address and data length */
	val =  addr << EFUSE_CTRL_ADDR | (wlen - 1) << EFUSE_CTRL_LEN;
	efuse_writel(val, EFUSE_CTRL);

	val = efuse_readl(EFUSE_CTRL);
	val &= ~EFUSE_CTRL_PD;
	efuse_writel(val, EFUSE_CTRL);

	/* enable read */
	val = efuse_readl(EFUSE_CTRL);
	val |= EFUSE_CTRL_RDEN;
	efuse_writel(val, EFUSE_CTRL);

	//printf("efuse ctrl regval=0x%x\n",val);
	/* wait read done status */
	while(!(efuse_readl(EFUSE_STATE) & EFUSE_STA_RD_DONE));

	efuse_writel(0, EFUSE_CTRL);
	efuse_writel(EFUSE_CTRL_PD, EFUSE_CTRL);
}

static void rir_w(uint32_t addr, uint32_t value)
{
	unsigned int val;

	efuse_writel(value, EFUSE_DATA(0));
	efuse_writel(0, EFUSE_CTRL);

	val =  addr << EFUSE_CTRL_ADDR | 0 << EFUSE_CTRL_LEN;
	efuse_writel(val, EFUSE_CTRL);

	val = efuse_readl(EFUSE_CTRL);
	val &= ~EFUSE_CTRL_PD;
	efuse_writel(val, EFUSE_CTRL);

	val = efuse_readl(EFUSE_CTRL);
	val |= EFUSE_CTRL_PS | EFUSE_CTRL_RWL;
	efuse_writel(val, EFUSE_CTRL);

	val = efuse_readl(EFUSE_CTRL);
	val |= EFUSE_CTRL_PGEN;
	efuse_writel(val, EFUSE_CTRL);

	boost_vddq(efuse_gpio);

	val = efuse_readl(EFUSE_CTRL);
	val |= EFUSE_CTRL_WREN;
	efuse_writel(val, EFUSE_CTRL);

	/* wait write done status */
	while(!(efuse_readl(EFUSE_STATE) & EFUSE_STA_WR_DONE));

	reduce_vddq(efuse_gpio);

	efuse_writel(0, EFUSE_CTRL);
	efuse_writel(EFUSE_CTRL_PD, EFUSE_CTRL);
}

static void rir_r(void)
{
	unsigned int val;

	efuse_writel(0, EFUSE_CTRL);
	efuse_writel(0, EFUSE_DATA(0));
	efuse_writel(0, EFUSE_DATA(1));

	/* set rir read address and data length */
	val =  0x1f << EFUSE_CTRL_ADDR | 0x1 << EFUSE_CTRL_LEN;
	efuse_writel(val, EFUSE_CTRL);

	val = efuse_readl(EFUSE_CTRL);
	val &= ~EFUSE_CTRL_PD;
	efuse_writel(val, EFUSE_CTRL);

	val = efuse_readl(EFUSE_CTRL);
	val &= ~EFUSE_CTRL_PS;
	efuse_writel(val, EFUSE_CTRL);

	val = efuse_readl(EFUSE_CTRL);
	val |= EFUSE_CTRL_RWL;
	efuse_writel(val, EFUSE_CTRL);

	val = efuse_readl(EFUSE_CTRL);
	val |= EFUSE_CTRL_RDEN;
	efuse_writel(val, EFUSE_CTRL);

	/* wait read done status */
	while(!(efuse_readl(EFUSE_STATE) & EFUSE_STA_RD_DONE));

	efuse_writel(0, EFUSE_CTRL);
	efuse_writel(EFUSE_CTRL_PD, EFUSE_CTRL);

	printf("RIR0=0x%08x\n", efuse_readl(EFUSE_DATA(0)));
	printf("RIR1=0x%08x\n", efuse_readl(EFUSE_DATA(1)));
}

static int rir_op(uint32_t value, uint32_t flag)
{
	unsigned int addr = 0, rf_addr = 0;
	unsigned int fb_disable = 0;
	unsigned int ret1, ret2;
	int rir_num = 0;

	if(value == 0)
		return -1;

	rir_r();
	ret1 = efuse_readl(EFUSE_DATA(0));
	ret2 = efuse_readl(EFUSE_DATA(1));

	if((ret1 & 0xFFFF) && (ret1 & (0xFFFF << 16)) &&
			(ret2 & 0xFFFF) && (ret2 & (0xFFFF << 16))) {
		if(flag == 1) {
			fb_disable = 0x1 << 31;
			rf_addr = 0x20;
			rir_w(rf_addr, fb_disable);
		}
		printf("not redundancy bits!\n");
		return -1;
	}

	if(((ret1 & (0xFFFF)) && (ret1 & (0xFFFF << 16)))) {
		addr = 0x20;
		if(ret2 & 0xFFFF) {
			value = value << 16;
			rir_num = 4;
		} else {
			rir_num = 3;
		}
	} else {
		addr = 0;
		if(ret1 & 0xFFFF) {
			value = value << 16;
			rir_num = 2;
		} else {
			rir_num = 1;
		}
	}

	if(flag == 1) {
		switch(rir_num) {
			case 2:
				fb_disable = 0x1 << 15;
				rf_addr = 0x0;
				break;
			case 3:
				fb_disable = 0x1 << 31;
				rf_addr = 0x0;
				break;
			case 4:
				fb_disable = 0x1 << 15;
				rf_addr = 0x20;
				break;
			default:
				printf("not rir %d!\n", rir_num);
				return -1;
		}

		rir_w(rf_addr, fb_disable);
	}

	rir_w(addr, value);

	return 0;
}

static int rir_check(struct seg_info *info, uint32_t woffs, uint32_t val)
{
	unsigned int rval, errbits;

	rir_r();
	otp_r(info->word_address + woffs, 1);

	rval = efuse_readl(EFUSE_DATA(0));
	if(woffs == 0)
		rval &= 0xffffffff << info->begin_align * 8;
	else if(woffs == info->word_num - 1)
		rval &= 0xffffffff >> info->end_align* 8;

	printf("%08x ^ %08x\n", rval, val);
	errbits = rval ^ val;

	return errbits;
}

static int rir_repair(struct seg_info *info, uint32_t *buf)
{
	unsigned int errbits, rir_data, repair_result, repair_fail;
	int ret, n, ebit;

	for(n = 0; n < info->word_num; n++) {
		errbits = rir_check(info, n, buf[n]);
		printf("addr=%x, errbits=0x%08x\n", info->word_address + n, errbits);

		while((ebit = (ffs(errbits)-1)) > 0) {
			rir_data = 0x1 << EFUSE_RIR_RF;
			rir_data |= (buf[n] & (0x1 << ebit)) << EFUSE_RIR_DATA;
			rir_data |= (info->word_address + n + (ebit << 6)) << EFUSE_RIR_ADDR;
//			rir_data &= 0 << EFUSE_RIR_DISABLE;

			ret = rir_op(rir_data, 0);
			if(ret) {
				printf("rir repair failed!\n");
				return -1;
			}

			do {
				repair_result = rir_check(info, n, buf[n]);
				repair_fail = repair_result & (0x1 << ebit);
				if(repair_fail) {
					ret = rir_op(rir_data, 1);
					if(ret) {
						printf("rir repair failed!\n");
						return -1;
					}
				}
			} while(repair_fail);
			errbits &= ~(0x1 << ebit);
		}
	}

	return 0;
}

static void rir_disable_all(void)
{
	rir_r();
	rir_w(0x0, (1 << 15));
	rir_w(0x0, (1 << 31));
	rir_w(0x20, (1 << 15));
	rir_w(0x20, (1 << 31));
}

static int jz_efuse_read(struct seg_info *info, uint32_t *buf)
{
	uint32_t val;
	uint32_t rbuf[8] = {0};
	uint32_t tmp[8] = {0};
	uint32_t byte_num = 0;
	uint32_t half_bit_num = 0;
	uint32_t hamming_bit_num = 0;
	uint32_t hamming_byte_num = 0;
	int n, ret;

	printf("segment name: %s\nsegment addr: 0x%02x\nbegin align: %d\nend align: %d\n"
			"word num: %d\nbit num: %d\nverify mode: %d\n",
			info->seg_name, info->word_address, info->begin_align, info->end_align,
			info->word_num, info->bit_num, info->verify_mode);

	rir_r();
	otp_r(info->word_address, info->word_num);

	for(n = 0; n < info->word_num; n++) {
		val = efuse_readl(EFUSE_DATA(n));
		printf("%08x\n", val);
		if(n == 0)
			rbuf[n] = val & (0xffffffff << info->begin_align * 8);
		else if(n == info->word_num - 1)
			rbuf[n] = val & (0xffffffff >> info->end_align * 8);
		else
			rbuf[n] = val;
	}

	byte_num = info->bit_num / 8;
	byte_num += info->bit_num % 8 ? 1 : 0;

	switch(info->verify_mode) {
		case HAMMING:
			hamming_bit_num = info->bit_num + cal_k(info->bit_num);
			hamming_byte_num = hamming_bit_num / 8 + (hamming_bit_num % 8 ? 1 : 0);
//			dump(rbuf, 0, hamming_bit_num);
			memcpy((char*)tmp, (char *)rbuf + info->begin_align, byte_num + hamming_byte_num);
			decode(tmp, hamming_bit_num, buf);
			break;
		case DOUBLE:
			if (info->bit_num % 2) {
				printf("%s segment bits is not aliged!\n",info->seg_name);
				return -1;
			}
			half_bit_num = info->bit_num / 2;
			bit_copy(tmp, 0, rbuf, info->begin_align * 8 + half_bit_num, half_bit_num);
			ret = checkbit(rbuf, tmp, info->begin_align * 8, 0, half_bit_num);
			if(ret){
				printf("double verify failed!\n");
				return -1;
			}
			memcpy((char *)buf, ((char *)rbuf + info->begin_align), byte_num);
			break;
		case NONE:
		default:
			memcpy((char *)buf, ((char *)rbuf + info->begin_align), byte_num);
			break;
	}

	printf("efuse read data after decode :\n");
	for(n = 0; n < info->word_num; n++) {
		printf("%08x\n", buf[n]);
	}

	/* clear read done status */
	efuse_writel(0, EFUSE_STATE);
	efuse_writel(EFUSE_CTRL_PD, EFUSE_CTRL);

	return 0;
}


static void otp_w(uint32_t addr, uint32_t wlen)
{
	unsigned int val;

	efuse_writel(0, EFUSE_CTRL);

	/* set write Programming address and data length */
	val =  addr << EFUSE_CTRL_ADDR | (wlen - 1) << EFUSE_CTRL_LEN;
	efuse_writel(val, EFUSE_CTRL);

	val = efuse_readl(EFUSE_CTRL);
	val &= ~EFUSE_CTRL_PD;
	efuse_writel(val, EFUSE_CTRL);

	val = efuse_readl(EFUSE_CTRL);
	val |= EFUSE_CTRL_PS;
	efuse_writel(val, EFUSE_CTRL);

	/* Programming EFUSE enable */
	val = efuse_readl(EFUSE_CTRL);
	val |= EFUSE_CTRL_PGEN;
	efuse_writel(val, EFUSE_CTRL);

	/* Connect VDDQ pin from 1.8V */
	boost_vddq(efuse_gpio);

	/* enable write */
	val = efuse_readl(EFUSE_CTRL);
	val |= EFUSE_CTRL_WREN;
	efuse_writel(val, EFUSE_CTRL);

	/* wait write done status */
	while(!(efuse_readl(EFUSE_STATE) & EFUSE_STA_WR_DONE));

	/* Disconnect VDDQ pin from 1.8V. */
	reduce_vddq(efuse_gpio);

	efuse_writel(0, EFUSE_CTRL);
	efuse_writel(EFUSE_CTRL_PD, EFUSE_CTRL);
}

static int jz_efuse_write(struct seg_info *info, uint32_t *buf)
{
	unsigned int val[8] = {0};
	unsigned int tmp[8] = {0};
	uint32_t regval = 0;
	uint32_t byte_num = 0;
	uint32_t half_bit_num = 0;
	uint32_t hamming_bit_num = 0;
	uint32_t hamming_byte_num = 0;
	int ret = 0;
	int n = 0;


	printf("segment name: %s\nsegment addr: 0x%02x\nbegin align: %d\nend align: %d\n"
			"word num: %d\nbit num: %d\nverify mode: %d\n",
			info->seg_name, info->word_address, info->begin_align, info->end_align,
			info->word_num, info->bit_num, info->verify_mode);

	if(info->seg_id != PRT) {
		regval = efuse_readl(EFUSE_STATE);
		if(info->prt_bit & regval) {
			printf("segment[%s] has been protected!\n", info->seg_name);
			return -1;
		}
	}

	byte_num = info->bit_num / 8;
	byte_num += info->bit_num % 8 ? 1 : 0;

	switch(info->verify_mode) {
		case HAMMING:
			hamming_bit_num = info->bit_num + cal_k(info->bit_num);
			hamming_byte_num = hamming_bit_num / 8 + (hamming_bit_num % 8 ? 1 : 0);
//			dumphex(buf, info->word_num);
			encode(buf, info->bit_num, tmp);
			memcpy((char*)val + info->begin_align, (char*)tmp, byte_num + hamming_byte_num);
//			dump(tmp, 0, hamming_bit_num + info->begin_align * 8);
//			dumphex(tmp, info->word_num);
			break;
		case DOUBLE:
			if (info->bit_num % 2) {
				printf("%s segment bits is not aliged!\n",info->seg_name);
				return -1;
			}
			half_bit_num = info->bit_num / 2;
			ret = checkbit(buf, buf, 0, half_bit_num, half_bit_num);
			if(ret){
				printf("double verify failed!\n");
				return -1;
			}
			memcpy((char*)val + info->begin_align, (char *)buf, byte_num);
			break;
		case NONE:
		default:
			memcpy((char*)val + info->begin_align, (char *)buf, byte_num);
			break;
	}
	printf("efuse write data:\n");
	for(n = 0; n < info->word_num; n++) {
		printf("%08x\n", val[n]);
		efuse_writel(val[n], EFUSE_DATA(n));
	}

	otp_w(info->word_address, info->word_num);

	if(info->verify_mode == HAMMING) {
		ret = rir_repair(info, val);
		if(ret < 0){
			printf("hamming verify failed!\n");
			return -1;
		}
	}
	return 0;
}


static int adjust_efuse()
{

	uint32_t val, ns;
	int i, rd_strobe, wr_strobe;
	uint32_t rd_adj, wr_adj;
	int flag;
	int h2clk = clk_get_rate(H2CLK);

	ns = 1000000000 / h2clk;
	printf("rate = %d, ns = %d\n", h2clk, ns);

	for(i = 0; i <= 0xf; i++) {
		if((i + 1) * ns > 4)
			break;
	}
	if(i > 0xf) {
		printf("rd_adj and wr_adj fail!\n");
		return -1;
	}
	rd_adj = wr_adj = i;

	for(i = 0; i <= 0x1f; i++) {
		if(((rd_adj + i + 30) * ns) > 100)
			break;
	}
	if(i > 0x1f) {
		printf("get efuse cfg rd_strobe fail!\n");
		return -1;
	}
	rd_strobe = i;

	for(i = 0; i <= 0x3ff; i++) {
		val = (wr_adj + i + 3000) * ns;
		if(val > 13000) {
			val = (wr_adj - i + 3000) * ns;
			flag = 1;
		}

		if(val >= 11500 && val <= 12500)
			break;
	}

	if(i == 0x7ff) {
		printf("wr_strobe fail!\n");
		return -1;
	}

	wr_strobe = i;

	if(flag)
		wr_strobe |= (1 << 10);

	printf("rd_adj = %d | rd_strobe = %d | wr_adj = %d | wr_strobe = %d\n", 
		rd_adj, rd_strobe, wr_adj, wr_strobe);

	/*set configer register*/
	val = rd_adj << EFUSE_CFG_RD_ADJ | rd_strobe << EFUSE_CFG_RD_STROBE;
	val |= wr_adj << EFUSE_CFG_WR_ADJ | wr_strobe;
	efuse_writel(val, EFUSE_CFG);
	printf("h2clk is %d, efuse_reg 0x%x\n",h2clk,efuse_readl(EFUSE_CFG));
	return 0;
}

int efuse_read(void *buf, int length, off_t offset)
{
        int i = 0;
	int ret = -EPERM;
	uint32_t seg_id = 0;
	uint32_t seg_length = 0;
	char *last = NULL;
	uint32_t val[8] = {0};

	seg_length = sizeof(seg_info_array) / sizeof(seg_info_array[0]);

	if(offset < seg_info_array[0].word_address || offset >= seg_info_array[seg_length-1].word_address+seg_info_array[seg_length-1].word_num) {
	    printf("ERROR:offset is error\n");
	    return -EPERM;
	}

	for(i=0; i < seg_length; i++) {
	    if (offset >= seg_info_array[i].word_address && offset < seg_info_array[i].word_address + seg_info_array[i].word_num) {
		seg_id = i;
		break;
	    }
	}

	info = seg_info_array[seg_id];
	last = (char *)val + info.bit_num / 8 - 1;
	ret = jz_efuse_read(&info,val);
	if(ret < 0) {
	        printf("efuse_read_id: read id error\n");
		return ret;
	}

        for(i = 0; i < info.bit_num / 8; i++)
		snprintf((char *)buf + (i * 2), 3, "%02x", *((uint8_t *)last - i));
	strcat(buf, "\n");
	printf("read efuse data: %s\n",buf);
	return 0;
}

int efuse_read_id(void *buf, int length, int seg_id)
{
	int i = 0;
	int ret = -EPERM;
	char *last = NULL;
	char *ptr = buf;
	uint32_t val[8] = {0};
	info = seg_info_array[seg_id];
	last = (char *)val + info.bit_num / 8 - 1;
	*ptr = 0;

	ret = jz_efuse_read(&info,val);
	if(ret < 0) {
		printf("efuse_read_id: read id error\n");
		return ret;
	}

	if (seg_id == CHIPID) {
		for(i = (info.bit_num / 8 / 4 -1); i >=0 ; i--) {
			sprintf(ptr,"%08x", val[i]);
                        ptr = buf + strlen(buf);
		}
		ret = strlen(buf);
		printf("chipid :%s\n",buf);
	} else {
		for(i = 0; i < info.bit_num / 8; i++) {
			uint8_t byte = *((uint8_t *)last - i);
			sprintf(ptr, "%02x", byte);
			ptr += strlen(ptr);
		}
		ret = strlen(buf);
		printf("read efuse data: %s\n",buf);
	}

	return ret;
}
int efuse_write(void *buf, int length, off_t seg_id)
{
	int ret = -EPERM;
	int byte_num = 0;
	int word_num = 0;
	int left_num = 0;
	struct seg_info info;
	unsigned int prtbit = 0;
	unsigned int val[8] = {0};
	char tmp[9] = {'\0'};
	char *last = (char *)buf + length;
	int i = 0;

	if (IS_ERR(buf)) {
		printf("%s %d: buffer error!\n",__func__,__LINE__);
		return ret;
	}

	if (seg_id < 0 || seg_id > NKU) {
		printf("%s %d: segment id error!\n",__func__,__LINE__);
		return ret;
	}

	info = seg_info_array[seg_id];
	byte_num = length / 2;
	word_num = byte_num / 4;
	left_num = byte_num % 4;

	if (byte_num > info.bit_num / 8) {
		printf("%s %d: %s segment size error! %d %d\n",
				__func__,__LINE__,info.seg_name,info.bit_num,byte_num);
		return ret;
	}


	printf("%s %d: input %s\n",__func__,__LINE__,buf);
	for (i = 0; i < word_num; i++) {
		memcpy(tmp, last - ((i + 1) * 8), 8);
		val[i] = (unsigned int)simple_strtoul(tmp, NULL, 16);
	}

	if (left_num > 0)  {
		memset(tmp, 0, 8);
		memcpy(tmp, (char *)buf, left_num * 2);
		val[i] = (unsigned int)simple_strtoull(tmp, NULL, 16);
	}

	ret = jz_efuse_write(&info, val);
	if (ret != 0) {
		printf("%s %d: write error!\n",__func__,__LINE__);
		return ret;
	}

	return ret;
}


static int x2000_get_segment_by_offset(unsigned int offset, struct seg_info *seg,
		unsigned int *seg_start, unsigned int *seg_bytes)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(seg_info_array); i++) {
		unsigned int start = seg_info_array[i].word_address * 4 +
			seg_info_array[i].begin_align;
		unsigned int bytes = seg_info_array[i].bit_num / 8;

		bytes += seg_info_array[i].bit_num % 8 ? 1 : 0;
		if (offset >= start && offset < start + bytes) {
			if (seg)
				*seg = seg_info_array[i];
			if (seg_start)
				*seg_start = start;
			if (seg_bytes)
				*seg_bytes = bytes;
			return 0;
		}
	}

	return -EINVAL;
}

int efuse_read_segment(void *buf, int length, off_t offset)
{
	struct seg_info seg;
	unsigned int seg_start;
	unsigned int seg_bytes;
	unsigned int inner_offset;
	unsigned int storage_bytes;
	unsigned char *raw = NULL;
	int ret;

	if (!buf || length <= 0)
		return -EINVAL;

	ret = x2000_get_segment_by_offset(offset, &seg, &seg_start, &seg_bytes);
	if (ret)
		return ret;

	inner_offset = offset - seg_start;
	if (length > seg_bytes - inner_offset)
		length = seg_bytes - inner_offset;

	storage_bytes = seg.word_num * 4;
	raw = malloc(storage_bytes);
	if (!raw)
		return -ENOMEM;
	memset(raw, 0, storage_bytes);

	ret = jz_efuse_read(&seg, (uint32_t *)raw);
	if (ret < 0)
		goto out;

	memcpy(buf, raw + inner_offset, length);
	ret = length;
out:
	free(raw);
	return ret;
}

int efuse_write_segment(void *buf, int length, off_t offset)
{
	struct seg_info seg;
	unsigned int seg_start;
	unsigned int seg_bytes;
	unsigned int inner_offset;
	unsigned int storage_bytes;
	unsigned int max_input_length;
	unsigned char *raw = NULL;
	char *hex = NULL;
	int input_length;
	int hex_offset;
	int ret;

	if (!buf || length <= 0)
		return -EINVAL;

	ret = x2000_get_segment_by_offset(offset, &seg, &seg_start, &seg_bytes);
	if (ret)
		return ret;

	inner_offset = offset - seg_start;
	max_input_length = (seg_bytes - inner_offset) * 2;
	if (length > max_input_length)
		length = max_input_length;

	input_length = strnlen((char *)buf, length);
	if (!input_length)
		return -EINVAL;

	storage_bytes = seg.word_num * 4;
	raw = malloc(storage_bytes);
	hex = malloc(seg_bytes * 2 + 1);
	if (!raw || !hex) {
		ret = -ENOMEM;
		goto out;
	}
	memset(raw, 0, storage_bytes);

	ret = jz_efuse_read(&seg, (uint32_t *)raw);
	if (ret < 0)
		goto out;

	efuse_segment_bytes_to_hex(raw, seg_bytes, hex);
	hex_offset = (seg_bytes - inner_offset) * 2 - input_length;
	if (hex_offset < 0) {
		ret = -EINVAL;
		goto out;
	}
	memcpy(hex + hex_offset, buf, input_length);

	ret = efuse_write(hex, seg_bytes * 2, seg.seg_id);
out:
	free(raw);
	free(hex);
	return ret;
}

int efuse_init(int gpio_pin, int active)
{
	if(gpio_pin >= 0){
		if(efuse_gpio >= 0) gpio_free(efuse_gpio);
		efuse_gpio = gpio_request(gpio_pin, "VDDQ");
		if(efuse_gpio < 0) return efuse_gpio;
		efuse_en_active = active;
	}else{
		efuse_gpio = -1;
	}

	if(adjust_efuse() < 0)
		return -1;
	return 0;
}
void efuse_debug_enable(int enable)
{
	efuse_debug = !!enable;
	return;
}
