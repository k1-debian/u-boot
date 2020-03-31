#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/clk.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spinand.h>

#include <generated/sfc_timing_val.h>
#include <generated/sfc_nand_params.h>


//#define CONFIG_SPI_STANDARD
#define CONFIG_NAND_BPP             (2048)
#define CONFIG_NAND_PPB             (64)

static struct spl_nand_param *curr_device;

static inline void sfc_writel(unsigned int value, unsigned short offset)
{
	writel(value, SFC_BASE + offset);
}

static inline unsigned int sfc_readl(unsigned short offset)
{
	return readl(SFC_BASE + offset);
}

static void sfc_set_mode(unsigned int channel, unsigned int value)
{
	unsigned int tmp;

	tmp = sfc_readl(SFC_TRAN_CONF1(channel));
	tmp &= ~TRAN_CONF1_TRAN_MODE_MSK;
	tmp |= (value << TRAN_CONF1_TRAN_MODE_OFFSET);
	sfc_writel(tmp, SFC_TRAN_CONF1(channel));
}

static inline void sfc_set_length(unsigned int value)
{
	sfc_writel(value, SFC_TRAN_LEN);
}

static void sfc_transfer_direction(unsigned int value)
{
	unsigned int tmp;

	tmp = sfc_readl(SFC_GLB);

	if(value == 0)
		tmp &= ~GLB_TRAN_DIR;
	else
		tmp |= GLB_TRAN_DIR;

	sfc_writel(tmp, SFC_GLB);
}

static inline void sfc_dev_addr(unsigned int channel, unsigned int value)
{
	sfc_writel(value, SFC_DEV_ADDR(channel));
}

static inline void sfc_dev_addr_plus(unsigned int channel, unsigned int value)
{
	sfc_writel(value, SFC_DEV_ADDR_PLUS(channel));
}

static inline void set_flash_timing(void)
{
	sfc_writel(DEF_TIM_VAL, SFC_DEV_CONF);
}

static inline void sfc_tranconf_init(struct jz_sfc *sfc, unsigned int channel)
{
	sfc_writel(sfc->tranconf.d32, SFC_TRAN_CONF0(channel));
}

static void sfc_set_transfer(struct jz_sfc *sfc, unsigned int dir)
{
	sfc_set_mode(0, sfc->tran_mode);
	sfc_tranconf_init(sfc, 0);

	if(dir == 1)
		sfc_transfer_direction(GLB_TRAN_DIR_WRITE);
	else
		sfc_transfer_direction(GLB_TRAN_DIR_READ);
	sfc_set_length(sfc->len);
	sfc_dev_addr(0, sfc->addr);
}

static void clear_end(void)
{
	while(!(sfc_readl(SFC_SR) & END));
        sfc_writel(CLR_END, SFC_SCR);
}

static void sfc_send_cmd(struct jz_sfc *sfc, unsigned char dir)
{
	sfc_writel(1 << 1, SFC_TRIG);
	sfc_set_transfer(sfc, dir);
	sfc_writel(1 << 2, SFC_TRIG);
	sfc_writel(TRIG_START, SFC_TRIG);

	/*this must judge the end status*/
	//clear_end();
}

static void sfc_write_data(unsigned int *data, unsigned int length)
{
	while (!(sfc_readl(SFC_SR) & TRAN_REQ));
	sfc_writel(CLR_TREQ,SFC_SCR);
	sfc_writel(*data, SFC_RM_DR);
	clear_end();
}

static int sfc_read_data(unsigned int *data, unsigned int length)
{
	unsigned int tmp_len = 0;
	unsigned int fifo_num = 0;
	unsigned int reg_tmp = 0;
	unsigned int len = (length + 3) / 4;
	int i;

	while(1){
		reg_tmp = sfc_readl(SFC_SR);
		if (reg_tmp & RECE_REQ) {
			sfc_writel(CLR_RREQ, SFC_SCR);
			if ((len - tmp_len) > THRESHOLD)
				fifo_num = THRESHOLD;
			else
				fifo_num = len - tmp_len;

			for (i = 0; i < fifo_num; i++) {
				*data++ = sfc_readl(SFC_RM_DR);
				tmp_len++;
			}
		}
		if (tmp_len == len)
			break;
	}
	clear_end();

	return 0;
}

static void sfc_controler_init(void)
{
	unsigned int tmp;

	clk_set_rate(SFC, CONFIG_SFC_NAND_RATE);

	tmp = sfc_readl(SFC_GLB);
	tmp &= ~(GLB_THRESHOLD_MSK);
	tmp |= (THRESHOLD << GLB_THRESHOLD_OFFSET);
	sfc_writel(tmp, SFC_GLB);

	set_flash_timing();
}

static int spinand_bad_block_check(int len, unsigned char *check_buf)
{
	int i;

	for(i = 0; i < len; i++)
		if(check_buf[i] != 0xff)
			return 1;
	return 0;
}

static int spinand_read_page(unsigned int page, unsigned int column, unsigned char *dst_addr,
		unsigned int len, unsigned int pagesize)
{
	struct jz_sfc sfc;
	unsigned int read_buf = 0;
	int oob_flag = 0;
	unsigned char i;
	unsigned char checklen = 1;

read_oob:
	if (oob_flag) {
		column = pagesize;
		len = 4;
		dst_addr = (unsigned char *)&read_buf;
	}

	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_PARD, 0, page, 3, 0, 0, 0);
	clear_end();
	do {
		SFC_SEND_COMMAND(&sfc, SPINAND_CMD_GET_FEATURE, 1, SPINAND_ADDR_STATUS, 1, 0, 1, 0);
		sfc_read_data(&read_buf, 1);
	}while(read_buf & 0x1);
	/*ecc check*/

	for(i = 0; i < curr_device->eccstat_count; i++) {
		read_buf = read_buf & 0xff;
		if(((read_buf >> curr_device->ecc_bit) &
		(~(0xff << curr_device->bit_counts))) == curr_device->eccerrstatus[i])
			return -1;
	}

	/* plane select: MXIC 2G, Dosilicon 2G, Zetta 2G */
	if(curr_device->device_id == 0x20 || curr_device->device_id == 0x22 || (curr_device->device_id == 0x72))
		column |= (((page >> 6) & 1) << 12);

#ifndef CONFIG_SPI_STANDARD
	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_RDCH_X4, len, column, curr_device->addrlen, 8, 1, 0);
#else
	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_FRCH, len, column, curr_device->addrlen, 8, 1, 0);
#endif
	sfc_read_data((unsigned int *)dst_addr, len);

	if (!oob_flag && !(page % CONFIG_SPI_NAND_PPB)) {
		oob_flag = 1;
		goto read_oob;
	} else if (oob_flag) {
#if NAND_BUSWIDTH == NAND_BUSWIDTH_16
		checklen = 2;
#endif
		if (spinand_bad_block_check(checklen, (unsigned char *)&read_buf))
			return 1;
	}

	return 0;
}

static int probe_id_list(unsigned char* id)
{
	unsigned char i;

	for (i = 0; i < ARRAY_SIZE(nand_param); i++) {
		if (nand_param[i].id_manufactory == id[0] &&
			    nand_param[i].device_id == id[1]) {
			curr_device = &nand_param[i];
			break;
		}
	}

	if (i == ARRAY_SIZE(nand_param))
		return -ENODEV;

	return 0;
}

static int spinand_probe_id(struct jz_sfc* sfc)
{
	/*
	 * cmd-->addr-->pid
	 */
	unsigned char addrlen[] = {0, 1};
	unsigned char id[2] = {0};
	unsigned char i;

	for(i = 0; i < sizeof(addrlen); i++) {
		SFC_SEND_COMMAND(sfc, SPINAND_CMD_RDID, 2, 0, addrlen[i], 0, 1, 0);
		sfc_read_data((unsigned int *)id, 2);
		if (!probe_id_list(id))
			    break;
	}
	if(i == sizeof(addrlen)) {
		debug("ERR: don`t support this kind of nand device, \
			please add it\n");
		return -ENODEV;
	}

	printf("%d, VID=0x%x, PID=0x%x\n", __LINE__, id[0], id[1]);
	return 0;
}

int spinand_init(void)
{
	struct jz_sfc sfc;
	unsigned int x;

	/*
	 * Probe nand vid/pid
	 */
	if(spinand_probe_id(&sfc))
		return -ENODEV;

	/* disable write protect */
	x = 0;
	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_SET_FEATURE, 1, SPINAND_ADDR_PROTECT, 1, 0, 1, 1);
	sfc_write_data(&x, 1);

	x = BITS_QUAD_EN | BITS_ECC_EN | BITS_BUF_EN;
	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_SET_FEATURE, 1, SPINAND_ADDR_FEATURE, 1, 0, 1, 1);
	sfc_write_data(&x, 1);

	return 0;
}

int sfc_nand_load(unsigned int src_addr, unsigned int count, unsigned int dst_addr)
{
	unsigned int pageaddr, columnaddr, rlen;
	int ret, retry_count = 5;
	unsigned char *buf = (unsigned char *)dst_addr;
	unsigned int pagesize = curr_device->pagesize;

	while (count) {
		pageaddr = src_addr / pagesize;
		columnaddr = src_addr % pagesize;
		rlen = (pagesize - columnaddr) < count ? (pagesize - columnaddr) : count;

		ret = spinand_read_page(pageaddr, columnaddr, buf, rlen, pagesize);
		if (ret > 0) {
			debug("bad block %d\n", pageaddr / CONFIG_NAND_PPB);
			src_addr += CONFIG_NAND_PPB * pagesize;
			continue;
		}

		if(ret < 0 && retry_count--) {
			continue;
		}

		/*
		 *  current block ecc cannot be corrected after 5 retries,
		 *  skip this block as bad block.
		 **/
		if(retry_count < 0) {
			debug("read page ecc error, pageaddr = %d, columnaddr = %d\n", pageaddr,
				columnaddr);
			/* bad block */
			retry_count = 5;
			src_addr += CONFIG_NAND_PPB * pagesize;
			continue;
		}

		buf += rlen;
		src_addr += rlen;
		count -= rlen;
		retry_count = 5;
	}

	return 0;
}

void sfc_init(void)
{
	sfc_controler_init();
	spinand_init();
}

void spl_sfc_nand_load(void)
{
	struct image_header *header;

	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

	sfc_init();

	sfc_nand_load(CONFIG_UBOOT_OFFSET, CONFIG_SYS_MONITOR_LEN, (void *)CONFIG_SYS_TEXT_BASE);
	spl_parse_image_header(header);
}

char* spl_sfc_nand_load_image(void)
{
	spl_sfc_nand_load();
	return NULL;
}
