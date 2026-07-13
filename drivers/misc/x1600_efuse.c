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

static int efuse_debug = 1;
static int efuse_en_gpio = -1;
static int efuse_en_active = 0;


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


static void otp_r(uint32_t addr, uint32_t blen)
{
	unsigned int val;
	int n;

	efuse_writel(0, EFUSE_CTRL);

	for(n = 0; n < 8; n++)
		efuse_writel(0, EFUSE_DATA(n));

	/* set read address and data length */
	val =  addr << EFUSE_CTRL_ADDR | (blen - 1) << EFUSE_CTRL_LEN;
	efuse_writel(val, EFUSE_CTRL);

	/* enable read */
	val = efuse_readl(EFUSE_CTRL);
	val |= EFUSE_CTRL_RDEN;
	efuse_writel(val, EFUSE_CTRL);

	/* wait read done status */
	while(!(efuse_readl(EFUSE_STATE) & EFUSE_STA_RD_DONE));
}

static int jz_efuse_read(struct seg_info *info, uint8_t *buf)
{
	int i;
	unsigned int val = 0;
	int byte_num = info->bit_num / 8;
	int word_num = byte_num / 4;
	word_num += byte_num % 4 ? 1 : 0;


	printf("segment name: %s\nsegment addr: 0x%02x\nbyte num: %d\nbit num: %d\n",
			info->seg_name, info->offset_address, byte_num, info->bit_num);

	otp_r(info->offset_address, byte_num);

	debug_cond(efuse_debug, "efuse read data:\n");
	for (i = 0; i < word_num; i++) {
		val = efuse_readl(EFUSE_DATA(i));
		debug_cond(efuse_debug, "0x%08x\n", val);
		*((unsigned int *)buf + i) = val;
	}

	/* clear read done status */
	efuse_writel(0, EFUSE_STATE);

	return 0;
}

static void otp_w(uint32_t addr, uint32_t blen)
{
	unsigned int val;

	efuse_writel(0, EFUSE_CTRL);

	/* set  Programming address and data length */
	val =  addr << EFUSE_CTRL_ADDR | (blen - 1) << EFUSE_CTRL_LEN;
	efuse_writel(val, EFUSE_CTRL);

	/* Programming EFUSE enable */
	val = efuse_readl(EFUSE_CTRL);
	val |= EFUSE_CTRL_PGEN;
	efuse_writel(val, EFUSE_CTRL);

	/* Connect VDDQ pin from 2.5V */
	boost_vddq(efuse_en_gpio);
	mdelay(1);

	/* enable write */
	val = efuse_readl(EFUSE_CTRL);
	val |= EFUSE_CTRL_WREN;
	efuse_writel(val, EFUSE_CTRL);

	/* wait write done status */
	while(!(efuse_readl(EFUSE_STATE) & EFUSE_STA_WR_DONE));

	/* Disconnect VDDQ pin from 2.5V. */
	reduce_vddq(efuse_en_gpio);
	mdelay(1);

	val = efuse_readl(EFUSE_CTRL);
	val &= ~(EFUSE_CTRL_PGEN);
	efuse_writel(val, EFUSE_CTRL);
}

static int jz_efuse_write(struct seg_info *info, uint8_t *buf)
{
	int byte_num = info->bit_num / 8;
	int word_num = byte_num / 4;
	word_num += byte_num % 4 ? 1 : 0;
	unsigned int val = 0;
	int i;

	debug_cond(efuse_debug, "segment name: %s\nsegment addr: 0x%02x\nbyte num: %d\nbit num: %d\n",
			info->seg_name, info->offset_address, byte_num, info->bit_num);

	if (info->seg_id != PRT) {
		val = efuse_readl(EFUSE_STATE);
		if(info->prt_bit & val) {
			printf("segment[%s] has been protected!\n", info->seg_name);
			return -1;
		}
	}

	debug_cond(efuse_debug, "efuse write data:\n");
	for (i = 0; i < word_num; i++) {
		val = *((unsigned int *)buf + i);
		efuse_writel(val, EFUSE_DATA(i));
		debug_cond(efuse_debug, "0x%08x\n", val);
	}

	otp_w(info->offset_address, byte_num);

	return 0;
}

static int set_efuse_timing()
{
	unsigned long rate;
	uint32_t val, ns;
	int i, rd_strobe, wr_strobe;
	uint32_t rd_adj, wr_adj;
	int flag = 0;

	rate = clk_get_rate(H2CLK);
	ns = 1000000000 / rate;
	printf("rate = %lu, ns = %d\n", rate, ns);

	for(i = 0; i <= 0xf; i++) {
		if((i + 1) * ns > 7)
			break;
	}
	if(i > 0xf) {
		printf("rd_adj and wr_adj fail!\n");
		return -1;
	}
	rd_adj = wr_adj = i;

	for(i = 0; i <= 0x1f; i++) {
		if(((rd_adj + i + 5) * ns) > 35)
			break;
	}
	if(i > 0x1f) {
		printf("get efuse cfg rd_strobe fail!\n");
		return -1;
	}
	rd_strobe = i;

	for(i = 0; i <= 0x7ff; i++) {
		val = (wr_adj + i + 1666) * ns;
		if(val > 11000) {
			val = (wr_adj - i + 1666) * ns;
			flag = 1;
		}
		if(val > 9000 && val < 11000)
			break;
	}
	if(i > 0x7ff) {
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

	return 0;
}

int efuse_read(void *buf, int length, off_t seg_id)
{
	int ret = -EPERM;
	int byte_num = 0;
	struct seg_info info;

	if(IS_ERR(buf)) {
		printf("%s %d: buffer error!\n",__func__,__LINE__);
		return ret;
	}

	info = seg_info_array[seg_id];
	byte_num = info.bit_num / 8;

	if(length > byte_num) {
		printf("%s %d: %s segment size error! %d\n",__func__,__LINE__,info.seg_name,byte_num);
		return ret;
	}

	ret = jz_efuse_read(&info, buf);
	if(ret < 0) {
		printf("%s %d: read error!\n",__func__,__LINE__);
		return ret;
	}

	return ret;
}

int efuse_read_id(void *buf, int length, int seg_id)
{
	int ret = -EPERM;
	unsigned int val[8] = {0};
	struct seg_info info;
	info = seg_info_array[seg_id];
	int byte_num = info.bit_num / 8;
	char *last = (char *)val + byte_num - 1;
	int i = 0;

	if(IS_ERR(buf)) {
		printf("%s %d: buffer error!\n",__func__,__LINE__);
		return ret;
	}

	ret = jz_efuse_read(&info, val);
	if(ret < 0) {
		printf("%s %d: read error!\n",__func__,__LINE__);
		return ret;
	}

	for(i = 0; i < byte_num; i++) {
		snprintf((char *)buf + (i * 2), 3, "%02x", *((uint8_t *)last - i));
	}

	return strlen(buf);
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


static int x1600_get_segment_by_offset(unsigned int offset, struct seg_info *seg,
		unsigned int *seg_start, unsigned int *seg_bytes)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(seg_info_array); i++) {
		unsigned int start = seg_info_array[i].offset_address;
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

static void efuse_segment_bytes_to_hex(const unsigned char *raw,
		unsigned int raw_bytes, char *hex)
{
	unsigned int i;

	for (i = 0; i < raw_bytes; i++)
		sprintf(hex + (i * 2), "%02x", raw[raw_bytes - 1 - i]);
	hex[raw_bytes * 2] = '\0';
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

	ret = x1600_get_segment_by_offset(offset, &seg, &seg_start, &seg_bytes);
	if (ret)
		return ret;

	inner_offset = offset - seg_start;
	if (length > seg_bytes - inner_offset)
		length = seg_bytes - inner_offset;

	storage_bytes = ((seg_bytes + 3) / 4) * 4;
	raw = malloc(storage_bytes);
	if (!raw)
		return -ENOMEM;
	memset(raw, 0, storage_bytes);

	ret = jz_efuse_read(&seg, raw);
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

	ret = x1600_get_segment_by_offset(offset, &seg, &seg_start, &seg_bytes);
	if (ret)
		return ret;

	inner_offset = offset - seg_start;
	max_input_length = (seg_bytes - inner_offset) * 2;
	if (length > max_input_length)
		length = max_input_length;

	input_length = strnlen((char *)buf, length);
	if (!input_length)
		return -EINVAL;

	storage_bytes = ((seg_bytes + 3) / 4) * 4;
	raw = malloc(storage_bytes);
	hex = malloc(seg_bytes * 2 + 1);
	if (!raw || !hex) {
		ret = -ENOMEM;
		goto out;
	}
	memset(raw, 0, storage_bytes);

	ret = jz_efuse_read(&seg, raw);
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
		if(efuse_en_gpio >= 0) gpio_free(efuse_en_gpio);
		efuse_en_gpio = gpio_request(gpio_pin, "VDDQ");
		if(efuse_en_gpio < 0) return efuse_en_gpio;
		efuse_en_active = active;
	}else{
		efuse_en_gpio = -1;
	}

	if(set_efuse_timing() < 0)
		return -1;

	printf("%s %d: successful!\n",__func__,__LINE__);
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
