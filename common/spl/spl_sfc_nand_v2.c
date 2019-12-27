
#include <common.h>
#include <asm/io.h>
#include <asm/arch/clk.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spinand.h>

#include <generated/sfc_timing_val.h>


#define  CONFIG_SFC_FREQ            (110)
#undef CONFIG_SPI_STANDARD
#define CONFIG_NAND_BPP             (2048)
#define CONFIG_NAND_PPB             (64)


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

static void sfc_dev_addr_dummy_bits(unsigned int channel, unsigned int value)
{
	unsigned int tmp;

	tmp = sfc_readl(SFC_TRAN_CONF0(channel));
	tmp &= ~TRAN_CONF0_DMYBITS_MSK;
	tmp |= (value << TRAN_CONF0_DMYBITS_OFFSET);
	sfc_writel(tmp, SFC_TRAN_CONF0(channel));
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

	tmp = sfc_readl(SFC_TRAN_CONF0(channel));
	tmp &= ~(TRAN_CONF0_ADDR_WIDTH_MSK);
	tmp |= (value << TRAN_CONF0_ADDR_WIDTH_OFFSET);
	sfc_writel(tmp, SFC_TRAN_CONF0(channel));
}

static void sfc_cmd_en(unsigned int channel, unsigned int value)
{
	unsigned int tmp;

	tmp = sfc_readl(SFC_TRAN_CONF0(channel));

	if(value == 1)
		tmp |= TRAN_CONF0_CMDEN;
	else
		tmp &= ~TRAN_CONF0_CMDEN;

	sfc_writel(tmp, SFC_TRAN_CONF0(channel));
}

static void sfc_data_en(unsigned int channel, unsigned int value)
{
	unsigned int tmp;

	tmp = sfc_readl(SFC_TRAN_CONF0(channel));

	if(value == 1)
		tmp |= TRAN_CONF0_DATEEN;
	else
		tmp &= ~TRAN_CONF0_DATEEN;

	sfc_writel(tmp, SFC_TRAN_CONF0(channel));
}

static void sfc_write_cmd(unsigned int channel, unsigned int value)
{
	unsigned int tmp;

	tmp = sfc_readl(SFC_TRAN_CONF0(channel));
	tmp &= ~TRAN_CONF0_CMD_MSK;
	tmp |= value;
	sfc_writel(tmp, SFC_TRAN_CONF0(channel));
}

static inline void sfc_dev_addr(unsigned int channel, unsigned int value)
{
	sfc_writel(value, SFC_DEV_ADDR(channel));
}

static inline void sfc_dev_addr_plus(unsigned int channel, unsigned int value)
{
	sfc_writel(value, SFC_DEV_ADDR_PLUS(channel));
}

static inline void set_flash_timing()
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
	sfc_dev_addr_dummy_bits(0, sfc->dummy_byte);
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
		reg_tmp = sfc_readl(SFC_SR);
		while (!(reg_tmp & END))
			reg_tmp = sfc_readl(SFC_SR);

		if ((sfc_readl(SFC_SR)) & END)
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

	reg_tmp = sfc_readl(SFC_SR);
	while (!(reg_tmp & END))
		reg_tmp = sfc_readl(SFC_SR);

	if ((sfc_readl(SFC_SR)) & END)
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
				*data = sfc_readl(SFC_RM_DR);
				data++;
				tmp_len++;
			}
		}
		if (tmp_len == len)
			break;
	}

	reg_tmp = sfc_readl(SFC_SR);
	while (!(reg_tmp & END))
		reg_tmp = sfc_readl(SFC_SR);

	if ((sfc_readl(SFC_SR)) & END)
		sfc_writel(CLR_END, SFC_SCR);

	return 0;
}

void sfc_init(void)
{
	unsigned int tmp;
	int i;

	clk_set_rate(SFC, CONFIG_SFC_RATE);

	tmp = sfc_readl(SFC_GLB);
	tmp &= ~(GLB_THRESHOLD_MSK);
	tmp |= (THRESHOLD << GLB_THRESHOLD_OFFSET);
	sfc_writel(tmp, SFC_GLB);

	set_flash_timing();
}



static unsigned char gd5fxfq4xc_series;
static unsigned char gd5fxgq4xbxig_series;
static unsigned char addr_len;

static struct spi_mode_peer spi_mode_local[] = {
	[SPI_MODE_STANDARD] = {TRAN_CONF1_SPI_STANDARD, SPINAND_CMD_RDCH},
	[SPI_MODE_STANDARD2] = {TRAN_CONF1_SPI_STANDARD, SPINAND_CMD_FRCH},
	[SPI_MODE_QUAD] = {TRAN_CONF1_SPI_QUAD, SPINAND_CMD_RDCH_X4},
};

static struct special_spiflash_id spiflash_id[] = {
	/* ======================== GD5FxGQ4xC ======================= */
	{ GIGADEVICE_VID, GD5F1GQ4UC_PID },
	{ GIGADEVICE_VID, GD5F2GQ4UC_PID },
	{ GIGADEVICE_VID, GD5F1GQ4RC_PID },
	{ GIGADEVICE_VID, GD5F2GQ4RC_PID },
	/* ====================== GD5FxGQ4xC end ===================== */
};

static struct special_spiflash_desc spinand_descs[] = {
	{
		WINBOND_VID,
		{
			SPINAND_ADDR_FEATURE,//0xb0
			BITS_BUF_EN,
			VALUE_SET
		}
	},
};

static int spinand_bad_block_check(int len,unsigned char *buf)
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



int spinand_read_page(unsigned int page, unsigned char *dst_addr,
		unsigned int pagesize, unsigned int blksize)
{
	unsigned int read_buf;
	int column = 0;
	int oob_flag = 0;
	struct jz_sfc sfc;
	unsigned char error = 0;
	unsigned char checklen = 1;

read_oob:
	if (oob_flag) {
		column = pagesize;
		pagesize = 4;
		dst_addr = (unsigned char *)&read_buf;
	}

	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_PARD, 0, page, 3, 0, 0, 0);

	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_GET_FEATURE, 1, SPINAND_ADDR_STATUS, 1, 0, 1, 0);
	sfc_read_data(&read_buf, 1);
	while((read_buf & 0x1)) {
		SFC_SEND_COMMAND(&sfc, SPINAND_CMD_GET_FEATURE, 1, SPINAND_ADDR_STATUS, 1, 0, 1, 0);
		sfc_read_data(&read_buf, 1);
	}

	if (gd5fxgq4xbxig_series) {
		if ((read_buf >> 4) == 0x02)
			error = 1;

	} else if (gd5fxfq4xc_series){
		if ((read_buf >> 4) == 0x07)
			error = 1;
	} else {
		if(read_buf & 0x20)
			error = 1;
	}

	if (error) {
		printf("ecc error at page %d\n", page);
		return -1;
	}

	column = (column << 8) & 0xffffff00;
#ifndef CONFIG_SPI_STANDARD
	SFC_SEND_COMMAND(&sfc, SPI_MODE_QUAD, pagesize, column, addr_len, 0, 1, 0);
#else
	if (addr_len == 4)
		SFC_SEND_COMMAND(&sfc, SPI_MODE_STANDARD2, pagesize, column, addr_len, 0, 1, 0);
	else
		SFC_SEND_COMMAND(&sfc, SPI_MODE_STANDARD, pagesize, column, addr_len, 0, 1, 0);
#endif
	sfc_read_data((unsigned int *)dst_addr, pagesize);

	if (!oob_flag && !(page % CONFIG_NAND_PPB)) {
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
int spinand_read_page_4(unsigned int page, unsigned char *dst_addr,
		unsigned int pagesize, unsigned int blksize)
{
	unsigned int read_buf;
	int column = 0;
	int oob_flag = 0;
	struct jz_sfc sfc;
	unsigned char error = 0;
	unsigned char checklen = 1;

read_oob:
	if (oob_flag) {
		column = pagesize;
		pagesize = 4;
		dst_addr = (unsigned char *)&read_buf;
	}

	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_PARD, 0, page, 3, 0, 0, 0);

	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_GET_FEATURE, 1, SPINAND_ADDR_STATUS, 1, 0, 1, 0);
	sfc_read_data(&read_buf, 1);
	while((read_buf & 0x1)) {
		SFC_SEND_COMMAND(&sfc, SPINAND_CMD_GET_FEATURE, 1, SPINAND_ADDR_STATUS, 1, 0, 1, 0);
		sfc_read_data(&read_buf, 1);
	}

	if (gd5fxgq4xbxig_series) {
		if ((read_buf >> 4) == 0x02)
			error = 1;

	} else if (gd5fxfq4xc_series){
		if ((read_buf >> 4) == 0x07)
			error = 1;
	} else {
		if(read_buf & 0x20)
			error = 1;
	}

	if (error) {
		printf("ecc error at page %d\n", page);
		return -1;
	}

	column = (column << 8) & 0xffffff00;
	SFC_SEND_COMMAND(&sfc, SPI_MODE_QUAD, pagesize, column, 2, 8, 1, 0);

	sfc_read_data((unsigned int *)dst_addr, pagesize);

	if (!oob_flag && !(page % CONFIG_NAND_PPB)) {
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

static void spinand_dev_special_init(struct jz_sfc *sfc, unsigned int vid)
{
	struct spiflash_register *regs;
	unsigned int x = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(spinand_descs); i++) {
		if (vid == spinand_descs[i].vid) {
			regs = &spinand_descs[i].regs;
			SFC_SEND_COMMAND(sfc, SPINAND_CMD_GET_FEATURE, 1, regs->addr, 1, 0, 1, 0);
			sfc_read_data(&x, 1);
			OPERAND_CONTROL(regs->action, regs->val, x);
			SFC_SEND_COMMAND(sfc, SPINAND_CMD_SET_FEATURE, 1, regs->addr, 1, 0, 1, 1);
			sfc_write_data(&x, 1);
			SFC_SEND_COMMAND(sfc, SPINAND_CMD_GET_FEATURE, 1, regs->addr, 1, 0, 1, 0);
			sfc_read_data(&x, 1);
		}
	}
}

static unsigned char probe_id_list(unsigned char* id)
{
	unsigned char i;

	for (i = 0; i < ARRAY_SIZE(spiflash_id); i++) {
		if (spiflash_id[i].vid == id[0] &&
				spiflash_id[i].pid == id[1])
			break;
	}

	if (i == ARRAY_SIZE(spiflash_id))
		return 0;

	return 1;
}

static void spinand_probe_id(struct jz_sfc* sfc, unsigned char* id)
{
	/*
	 * cmd-->addr-->pid
	 */
	SFC_SEND_COMMAND(sfc, SPINAND_CMD_RDID, 2, 0, 1, 0, 1, 0);
	sfc_read_data((unsigned int *)id, 2);

	if (probe_id_list(id))
		goto id_found_out;

	else {
		/*
		 * cmd-->vid-->pid
		 */
		SFC_SEND_COMMAND(sfc, SPINAND_CMD_RDID, 3, 0, 0, 0, 1, 0);
		sfc_read_data((unsigned int *)id, 3);

		if (probe_id_list(id))
			goto id_found_out;

		else {
			/*
			 * cmd-->addr-->pid
			 */
			SFC_SEND_COMMAND(sfc, SPINAND_CMD_RDID, 2, 0, 1, 0, 1, 0);
			sfc_read_data((unsigned int *)id, 2);

			addr_len = 3;
			if (id[0] == GIGADEVICE_VID)
				gd5fxgq4xbxig_series = 1;

			return;
		}
	}

id_found_out:
	addr_len = 4;
	if (id[0] == GIGADEVICE_VID)
		gd5fxfq4xc_series = 1;
}

int spinand_init(void)
{
	unsigned char id[4] = {0, 0, 0, 0};
	unsigned int x;
	struct jz_sfc sfc;

	/*
	 * Probe nand vid/pid
	 */
	spinand_probe_id(&sfc, id);

	printf("%d, VID=0x%x, PID=0x%x\n", __LINE__, id[0], id[1]);

	/* disable write protect */
	x = 0;
	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_SET_FEATURE, 1, SPINAND_ADDR_PROTECT, 1, 0, 1, 1);
	sfc_write_data(&x, 1);

#ifndef CONFIG_SPI_STANDARD
	x = BITS_QUAD_EN;
#endif
	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_SET_FEATURE, 1, SPINAND_ADDR_FEATURE, 1, 0, 1, 1);
	sfc_write_data(&x, 1);

	spinand_dev_special_init(&sfc, id[0]);

	return 0;
}

static int sfc_nand_load(unsigned int src_addr, unsigned int count, unsigned int dst_addr)
{
	unsigned int blksize, pagesize, page;
	unsigned int pagecopy_cnt = 0;
	unsigned int ret;
	unsigned char *buf = (unsigned char *)dst_addr;

	pagesize = CONFIG_NAND_BPP;
	blksize = CONFIG_NAND_PPB * pagesize;

	if (src_addr % pagesize)
		printf("\n\tWarning: offset 0x%x not align with page size 0x%x.\n",
				src_addr, pagesize);

	page = src_addr / pagesize;
	while (pagecopy_cnt * pagesize < count) {
		ret = spinand_read_page(page, buf, pagesize, blksize);
		if (ret > 0){
			printf("bad block %d\n", page / CONFIG_NAND_PPB);
			page += CONFIG_NAND_PPB;
			continue;

		} else if (ret < 0)
			return -1;

		buf += pagesize;
		page++;
		pagecopy_cnt++;
	}

	return 0;
}

void spl_sfc_nand_load_image(void)
{
	struct image_header *header;
	char cmd[5] = {0};
	char idcode[5] = {0};
	unsigned int i=0;
	int chip_id;
	int try_times = 0;
	int id_dummy_nbits;
	int id_nbytes;
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

	sfc_init();
	spinand_init();

	spl_parse_image_header(header);
	sfc_nand_load(CONFIG_UBOOT_OFFSET,CONFIG_SYS_MONITOR_LEN,(void *)CONFIG_SYS_TEXT_BASE);
}
