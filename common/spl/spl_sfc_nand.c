/*#define DEBUG*/
#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <spl.h>
#include <asm/arch/clk.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spi_nand.h>
#include <generated/sfc_timing_val.h>
#include <generated/sfc_nand_params.h>

#ifdef CONFIG_SPL_OS_BOOT
struct jz_spinand_partition {
	char name[32];         /* identifier string */
	uint32_t size;          /* partition size */
	uint32_t offset;        /* offset within the master MTD space */
	u_int32_t mask_flags;       /* master MTD flags to mask out for this partition */
	u_int32_t manager_mode;     /* manager_mode mtd or ubi */
};

struct jz_nand_base_param {
	uint32_t pagesize;
	uint32_t blocksize;
	uint32_t oobsize;
	uint32_t flashsize;

	uint16_t tHOLD;
	uint16_t tSETUP;
	uint16_t tSHSL_R;
	uint16_t tSHSL_W;

	uint8_t ecc_max;
	uint8_t need_quad;
};

struct jz_sfc_nand_param {
	char name[32];
	short nand_id;
	struct jz_nand_base_param param;
};

struct jz_sfc_nand_burner_param {
	unsigned int magic_num;
	char version;
	struct jz_sfc_nand_param param;
	int partition_num;
	struct jz_spinand_partition *partition;

};
#endif

/*#define  CONFIG_SPI_STANDARD*/
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

	tmp = sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~(TRAN_MODE_MSK);
	tmp |= (value << TRAN_MODE_OFFSET);
	sfc_writel(tmp, SFC_TRAN_CONF(channel));
}

static void sfc_dev_dummy_bits(unsigned int channel, unsigned int value)
{
	unsigned int tmp;

	tmp = sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~TRAN_CONF_DMYBITS_MSK;
	tmp |= (value << TRAN_CONF_DMYBITS_OFFSET);
	sfc_writel(tmp, SFC_TRAN_CONF(channel));
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

static inline void sfc_set_length(unsigned int value)
{
	sfc_writel(value, SFC_TRAN_LEN);
}

static void sfc_set_addr_length(unsigned int channel, unsigned int value)
{
	unsigned int tmp;

	tmp = sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~(ADDR_WIDTH_MSK);
	tmp |= (value << ADDR_WIDTH_OFFSET);
	sfc_writel(tmp, SFC_TRAN_CONF(channel));
}

static void sfc_cmd_en(unsigned int channel, unsigned int value)
{
	unsigned int tmp;

	tmp = sfc_readl(SFC_TRAN_CONF(channel));

	if(value == 1)
		tmp |= CMDEN;
	else
		tmp &= ~CMDEN;

	sfc_writel(tmp, SFC_TRAN_CONF(channel));
}

static void sfc_data_en(unsigned int channel, unsigned int value)
{
	unsigned int tmp;

	tmp = sfc_readl(SFC_TRAN_CONF(channel));

	if(value == 1)
		tmp |= DATEEN;
	else
		tmp &= ~DATEEN;

	sfc_writel(tmp, SFC_TRAN_CONF(channel));
}

static void sfc_write_cmd(unsigned int channel, unsigned int value)
{
	unsigned int tmp;

	tmp = sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~CMD_MSK;
	tmp |= value;
	sfc_writel(tmp, SFC_TRAN_CONF(channel));
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

static void sfc_set_transfer(struct jz_sfc *sfc, unsigned int dir)
{
	if(dir == 1)
		sfc_transfer_direction(GLB_TRAN_DIR_WRITE);
	else
		sfc_transfer_direction(GLB_TRAN_DIR_READ);

	sfc_set_mode(0, sfc->sfc_mode);
	sfc_write_cmd(0, sfc->cmd);
	sfc_set_addr_length(0, sfc->addr_len);
	sfc_cmd_en(0, 0x1);
	sfc_data_en(0, sfc->daten);
	sfc_dev_dummy_bits(0, sfc->dummy_byte);
	sfc_set_length(sfc->len);
	sfc_dev_addr(0, sfc->addr);
}

static void sfc_send_cmd(struct jz_sfc *sfc, unsigned char dir)
{
	unsigned int reg_tmp = 0;

	sfc_writel(1 << 1, SFC_TRIG);
	sfc_set_transfer(sfc, dir);
	sfc_writel(1 << 2, SFC_TRIG);
	sfc_writel(TRIG_START, SFC_TRIG);

	/*this must judge the end status*/
	if((sfc->daten == 0)){
		while(!(sfc_readl(SFC_SR) & END));
		sfc_writel(CLR_END, SFC_SCR);
	}
}

static int sfc_write_data(unsigned int *data, unsigned int length)
{
	unsigned int tmp_len = 0;
	unsigned int fifo_num = 0;
	unsigned int reg_tmp = 0;
	unsigned int len = (length + 3) / 4 ;
	int i;

	while(1) {
		reg_tmp = sfc_readl(SFC_SR);
		if (reg_tmp & TRAN_REQ) {
			sfc_writel(CLR_TREQ,SFC_SCR);
			if ((len - tmp_len) > THRESHOLD)
				fifo_num = THRESHOLD;
			else
				fifo_num = len - tmp_len;

			for (i = 0; i < fifo_num; i++) {
				sfc_writel(*data, SFC_RM_DR);
				data++;
				tmp_len++;
			}
		}

		if (tmp_len == len)
			break;
	}

	while(!(sfc_readl(SFC_SR) & END));
	sfc_writel(CLR_END, SFC_SCR);

	return 0;
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

	while(!(sfc_readl(SFC_SR) & END));
	sfc_writel(CLR_END, SFC_SCR);

	return 0;
}

static void sfc_init(void)
{
	unsigned int tmp;

	clk_set_rate(SFC, CONFIG_SFC_NAND_RATE);

	tmp = sfc_readl(SFC_GLB);
	tmp &= ~(GLB_THRESHOLD_MSK);
	tmp |= (THRESHOLD << GLB_THRESHOLD_OFFSET);
	sfc_writel(tmp, SFC_GLB);

	set_flash_timing();
}

static int spinand_bad_block_check(int len, unsigned char *buf)
{
	int i, j, bit0_cnt = 0;
	unsigned char *check_buf = buf;

	for(j = 0; j < len; j++){
		if(check_buf[j] != 0xff){
			for(i = 0; i < 8; i++){
				if(!((check_buf[j] >> i) & 0x1))
					bit0_cnt++;
			}
		}
	}
	if(bit0_cnt > 6 * len)
		return 1;
	return 0;
}

static int spinand_read_page(unsigned int page, unsigned char *dst_addr,
			    unsigned int pagesize)
{
	struct jz_sfc sfc;
	unsigned int read_buf;
	int column = 0;
	int oob_flag = 0;
	unsigned char i;
	unsigned char checklen = 1;

read_oob:
	if (oob_flag) {
		column = pagesize;
		pagesize = 4;
		dst_addr = (unsigned char *)&read_buf;
	}

	SFC_SEND_COMMAND(&sfc, CMD_PARD, 0, page, 3, 0, 0, 0);

	SFC_SEND_COMMAND(&sfc, CMD_GET_FEATURE, 1, FEATURE_REG_STATUS1, 1, 0, 1, 0);
	sfc_read_data(&read_buf, 1);
	while((read_buf & 0x1)) {
		SFC_SEND_COMMAND(&sfc, CMD_GET_FEATURE, 1, FEATURE_REG_STATUS1, 1, 0, 1, 0);
		sfc_read_data(&read_buf, 1);
	}
	/*ecc check*/
	for(i = 0; i < curr_device->eccstat_count; i++) {
		if((read_buf >> curr_device->ecc_bit) &
		(~(0xff << curr_device->bit_counts)) == curr_device->eccerrstatus[i])
			return -1;
	}

#ifndef CONFIG_SPI_STANDARD
	SFC_SEND_COMMAND(&sfc, CMD_FR_CACHE_QUAD, pagesize, column, curr_device->addrlen, 8, 1, 0);
#else
	SFC_SEND_COMMAND(&sfc, CMD_FR_CACHE, pagesize, column, curr_device->addrlen, 8, 1, 0);
#endif
	sfc_read_data((unsigned int *)dst_addr, pagesize);

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

static int probe_id_list(unsigned char *id)
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

static int spinand_probe_id(struct jz_sfc *sfc)
{
	/*
	 * cmd-->addr-->pid
	 */
	unsigned char addrlen[] = {0, 1};
	unsigned char id[2] = {0};
	unsigned char i;

	for(i = 0; i < sizeof(addrlen); i++) {
		SFC_SEND_COMMAND(sfc, CMD_RDID, 2, 0, addrlen[i], 0, 1, 0);
		sfc_read_data((unsigned int *)id, 2);

		if (!probe_id_list(id))
			    break;
	}
	if(i == sizeof(addrlen)) {
		debug("ERR: don`t support this kind of nand device, \
			please add it\n");
		return -ENODEV;
	}
	return 0;
}

static int spinand_init(void)
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
	SFC_SEND_COMMAND(&sfc, CMD_SET_FEATURE, 1, FEATURE_REG_PROTECT, 1, 0, 1, 1);
	sfc_write_data(&x, 1);

	x = BITS_QUAD_EN | BITS_ECC_EN | BITS_BUF_EN;
	SFC_SEND_COMMAND(&sfc, CMD_SET_FEATURE, 1, FEATURE_REG_FEATURE1, 1, 0, 1, 1);
	sfc_write_data(&x, 1);

	SFC_SEND_COMMAND(&sfc, CMD_GET_FEATURE, 1, FEATURE_REG_FEATURE1, 1, 0, 1, 0);
	sfc_read_data(&x, 1);
	return 0;
}

static int sfc_nand_load(unsigned int src_addr, unsigned int count, unsigned int dst_addr)
{
	unsigned int page, pagesize;
	unsigned int pagecopy_cnt = 0;
	unsigned int ret, try_count = 5;
	unsigned char *buf = (unsigned char *)dst_addr;

	pagesize = curr_device->pagesize;

	if (src_addr % pagesize)
		debug("\n\tWarning: offset 0x%x not align with page size 0x%x.\n",
				src_addr, pagesize);

	page = src_addr / pagesize;
	while (pagecopy_cnt * pagesize < count) {
		ret = spinand_read_page(page, buf, pagesize);
		if (ret > 0) {
			debug("bad block %d\n", page / CONFIG_SPI_NAND_PPB);
			page += CONFIG_SPI_NAND_PPB;
			continue;
		} else if (ret < 0 && try_count--) {
		/*read page err, try again*/
			continue;
		}

		if (ret < 0 && try_count < 0) {
			debug("ERR: read page ECC error, page addr = %u\n", page);
			return -1;
		}

		buf += pagesize;
		page++;
		pagecopy_cnt++;
		try_count = 5;
	}

	return 0;
}


void spl_sfc_nand_load_image(void)
{
	struct image_header *header;
#ifdef CONFIG_SPL_OS_BOOT
	struct jz_sfc_nand_burner_param *burn_param;
	struct jz_spinand_partition *partition;
	unsigned int bootimg_addr = 0;
	unsigned int i = 0;
#endif
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

	sfc_init();
	if(spinand_init())
		return;

#ifdef CONFIG_SPL_OS_BOOT
	/*read burn param*/
	sfc_nand_load(CONFIG_SPIFLASH_PART_OFFSET, 1, CONFIG_SYS_TEXT_BASE);
	burn_param = (void *)(CONFIG_SYS_TEXT_BASE + CONFIG_SPIFLASH_PART_OFFSET % curr_device->pagesize);
	partition = &burn_param->partition;

	for(i = 0; i < burn_param->partition_num; i++) {
		if (!strncmp(partition[i].name, CONFIG_SPL_OS_NAME, sizeof(CONFIG_SPL_OS_NAME))) {
			bootimg_addr = partition[i].offset;
			break;
		}
	}

	/*read image head*/
	sfc_nand_load(bootimg_addr, 1, CONFIG_SYS_TEXT_BASE);
	spl_parse_image_header(header);
	sfc_nand_load(bootimg_addr, spl_image.size, spl_image.load_addr);
#else
	sfc_nand_load(CONFIG_UBOOT_OFFSET, CONFIG_SYS_MONITOR_LEN, (void *)CONFIG_SYS_TEXT_BASE);
	spl_parse_image_header(header);
#endif
}
