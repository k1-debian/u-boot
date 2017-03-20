#include <common.h>
#include <config.h>
#include <spl.h>
#include <asm/io.h>
#include <errno.h>
#include <linux/err.h>
#include <malloc.h>
#include <asm/arch/clk.h>
#include <div64.h>

#include <asm/arch/sfc_params.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spi_nor.h>
#include <generated/sfc_timing_val.h>


#define GS_RETRY_TIMES	100


struct sfc_flash flash[1];
struct sfc sfc[1];

static inline void sfc_writel(unsigned short offset, u32 value)
{
	writel(value, SFC_BASE + offset);
}

static inline unsigned int sfc_readl(unsigned short offset)
{
	return readl(SFC_BASE + offset);
}


static inline void sfc_flush_and_start(struct sfc *sfc)
{
	sfc_writel(SFC_TRIG, TRIG_FLUSH);
	sfc_writel(SFC_TRIG, TRIG_START);
}

static inline void sfc_clear_all_intc(struct sfc *sfc)
{
	sfc_writel(SFC_SCR, 0x1f);
}

static inline void sfc_mask_all_intc(struct sfc *sfc)
{
	sfc_writel(SFC_INTC, 0x1f);
}

static inline void sfc_set_length(struct sfc *sfc, int value)
{
	sfc_writel(SFC_TRAN_LEN, value);
}


static inline void sfc_read_rxfifo(struct sfc *sfc, unsigned int *value)
{
	*(volatile unsigned int*)value = sfc_readl(SFC_RM_DR);
}

static inline void sfc_write_txfifo(struct sfc *sfc, const unsigned int value)
{
	sfc_writel(SFC_RM_DR, value);
}


static inline unsigned int get_sfc_ctl_sr()
{
	return sfc_readl(SFC_SR);
}

static unsigned int cpu_read_rxfifo(struct sfc *sfc)
{
	int i;
	unsigned long align_len = 0;
	unsigned int fifo_num = 0;

	align_len = ALIGN(sfc->transfer->len, 4);

	if (((align_len - sfc->transfer->cur_len) / 4) > THRESHOLD) {
		fifo_num = THRESHOLD;
	} else {
		fifo_num = (align_len - sfc->transfer->cur_len) / 4;
	}

	for (i = 0; i < fifo_num; i++) {
		sfc_read_rxfifo(sfc, (unsigned int *)sfc->transfer->data);
		sfc->transfer->data += 4;
		sfc->transfer->cur_len += 4;
	}

	return 0;
}

static void cpu_write_txfifo(struct sfc *sfc)
{
	/**
	 * Assuming that all data is less than one word,
	 * if len large than one word, unsupport!
	 **/

	sfc_write_txfifo(sfc, *(unsigned int *)sfc->transfer->data);
}

static void sfc_sr_handle(struct sfc *sfc)
{
	unsigned int reg_sr = 0;
	unsigned int tmp = 0;
	while (1) {
		reg_sr = get_sfc_ctl_sr();
		if(reg_sr & CLR_END){
			tmp = CLR_END;
			break;
		}

		if (reg_sr & CLR_RREQ) {
			sfc_writel(SFC_SCR, CLR_RREQ);
			cpu_read_rxfifo(sfc);
		}

		if (reg_sr & CLR_TREQ) {
			sfc_writel(SFC_SCR, CLR_TREQ);
			cpu_write_txfifo(sfc);
		}

		if (reg_sr & CLR_UNDER) {
			tmp = CLR_UNDER;
			printf("UNDR!\n");
			break;
		}

		if (reg_sr & CLR_OVER) {
			tmp = CLR_OVER;
			printf("OVER!\n");
			break;
		}
	}
	if (tmp)
		sfc_writel(SFC_SCR, tmp);
}

static void sfc_start_transfer(struct sfc *sfc)
{
	sfc_clear_all_intc(sfc);
	sfc_mask_all_intc(sfc);
	sfc_flush_and_start(sfc);

	sfc_sr_handle(sfc);

}
static void sfc_phase_transfer(struct sfc *sfc,struct sfc_transfer *transfer)
{
	unsigned int tmp = 0;
	struct cmd_info *cmd = &transfer->cmd_info;

	tmp |= (transfer->addr_len << ADDR_WIDTH_OFFSET);	//addr_len
	tmp |= TRAN_CONF_CMDEN;	//cmd_enable
	tmp |= transfer->cmd_info.cmd;	//cmd
	tmp |= transfer->data_dummy_bits << DMYBITS_OFFSET;	//dummy
	if(transfer->cmd_info.dataen == 1) {	//date_enable
		tmp |= TRAN_CONF_DATEEN;
	} else {
		tmp &= ~TRAN_CONF_DATEEN;
	}
	tmp |= (transfer->sfc_mode << TRAN_CONF_TRAN_MODE_OFFSET);	//transfer_mode 0~8
	sfc_writel(SFC_TRAN_CONF0, tmp);
	sfc_writel(SFC_DEV_ADDR0, transfer->addr);	//addr
	sfc_writel(SFC_DEV_ADDR_PLUS0, transfer->addr_plus);	//addr_plus
}

static void sfc_glb_info_config(struct sfc *sfc,struct sfc_transfer *transfer)
{

	unsigned int tmp;
	tmp = sfc_readl(SFC_GLB);
	if(transfer->direction == GLB_TRAN_DIR_READ) {	//read or write direction
		tmp &= ~GLB_TRAN_DIR;
	} else {
		tmp |= GLB_TRAN_DIR;
	}
	tmp &= ~(GLB_OP_MODE << GLB_OP_MODE); //use CPU mode
	tmp &= ~GLB_PHASE_NUM_MSK;	//phase_num=1
	tmp |= 1 << GLB_PHASE_NUM_OFFSET;
	sfc_writel(SFC_GLB, tmp);

	sfc_set_length(sfc, transfer->len);
}

static void sfc_sync(struct sfc *sfc)
{
	struct sfc_transfer *xfer;

	xfer = sfc->transfer;

	sfc_phase_transfer(sfc,xfer);
	sfc_glb_info_config(sfc,xfer);
	sfc_start_transfer(sfc);
}

static int get_norflash_status(int command, int len)
{
	struct sfc_transfer transfer;
	int val = 0;

	memset(&transfer, 0, sizeof(transfer));

	transfer.cmd_info.cmd = command;
	transfer.cmd_info.dataen = ENABLE;
	transfer.len = len;
	transfer.data = &val;
	transfer.ops_mode = CPU_OPS;
	transfer.sfc_mode = TM_STD_SPI;
	transfer.direction = GLB_TRAN_DIR_READ;
	flash->sfc->transfer = &transfer;

	sfc_sync(flash->sfc);

	return val;

}
static void write_enable()
{
	struct sfc_transfer transfer;
	struct mini_spi_nor_info *spi_nor_info;
	struct spi_nor_cmd_info *wr_en;

	spi_nor_info = &flash->g_nor_info;
	wr_en = &spi_nor_info->wr_en;

	memset(&transfer, 0, sizeof(transfer));

	transfer.cmd_info.cmd = wr_en->cmd;
	transfer.cmd_info.dataen = DISABLE;
	transfer.addr_len = wr_en->addr_nbyte;
	transfer.sfc_mode = wr_en->transfer_mode;
	transfer.data_dummy_bits = wr_en->dummy_byte;

	flash->sfc->transfer = &transfer;
	sfc_sync(flash->sfc);
}

static void enter_4byte()
{
	struct sfc_transfer transfer;
	struct mini_spi_nor_info *spi_nor_info;
	struct spi_nor_cmd_info *en4byte;

	spi_nor_info = &flash->g_nor_info;
	en4byte = &spi_nor_info->en4byte;
	memset(&transfer, 0, sizeof(transfer));

	transfer.cmd_info.cmd = en4byte->cmd;
	transfer.cmd_info.dataen = DISABLE;
	transfer.addr_len = en4byte->addr_nbyte;
	transfer.sfc_mode = en4byte->transfer_mode;
	transfer.data_dummy_bits = en4byte->dummy_byte;
	flash->sfc->transfer = &transfer;
	sfc_sync(flash->sfc);
}

static void inline set_quad_mode_cmd()
{
	struct mini_spi_nor_info *spi_nor_info;

	spi_nor_info = &flash->g_nor_info;
	flash->cur_r_cmd = &spi_nor_info->read_quad;
}

/* write nor flash status register QE bit to set quad mode */
static int set_quad_mode_reg()
{
	unsigned int data;
	int ret;
	unsigned int times = GS_RETRY_TIMES;
	unsigned int val;
	struct sfc_transfer transfer;
	struct mini_spi_nor_info *spi_nor_info;
	struct spi_nor_st_info *quad_set;
	struct spi_nor_st_info *quad_get;
	struct spi_nor_st_info *busy;

	spi_nor_info = &flash->g_nor_info;
	quad_set = &spi_nor_info->quad_set;
	quad_get = &spi_nor_info->quad_get;
	busy = &spi_nor_info->busy;
	data = (quad_set->val & quad_set->mask) << quad_set->bit_shift;

	write_enable(flash);

	memset(&transfer, 0, sizeof(transfer));
	/* write ops */
	transfer.cmd_info.cmd = quad_set->cmd;
	transfer.cmd_info.dataen = ENABLE;
	transfer.len = quad_set->len;
	transfer.data = (unsigned char *)&data;
	transfer.data_dummy_bits = quad_set->dummy;
	transfer.ops_mode = CPU_OPS;
	transfer.sfc_mode = TM_STD_SPI;
	transfer.direction = GLB_TRAN_DIR_WRITE;
	flash->sfc->transfer = &transfer;
	sfc_sync(flash->sfc);

	while (times--) {
		val = (get_norflash_status(quad_get->cmd, quad_get->len) >> quad_get->bit_shift) & quad_get->mask;
		if (val == quad_get->val) {
		flash->cur_r_cmd = &spi_nor_info->read_quad;
			break;
		}
	}

	while(!(((get_norflash_status(busy->cmd, busy->len) >> busy->bit_shift) & busy->mask) == busy->val));
	return ret;

}

static void sfc_nor_read_params(unsigned int addr, unsigned char *buf, unsigned int len)
{
	struct sfc_transfer transfer;

	memset(&transfer, 0, sizeof(transfer));

	transfer.cmd_info.cmd = SPINOR_OP_READ;
	transfer.cmd_info.dataen = ENABLE;
	transfer.addr_len = DEF_ADDR_LEN;
	transfer.data_dummy_bits = 0;
	transfer.addr = addr;
	transfer.len = len;
	transfer.data = buf;
	transfer.cur_len = 0;
	transfer.ops_mode = CPU_OPS;
	transfer.sfc_mode = TM_STD_SPI;
	transfer.direction = GLB_TRAN_DIR_READ;

	flash->sfc->transfer = &transfer;
	sfc_sync(flash->sfc);

}

static inline void set_flash_timing()
{
	sfc_writel(SFC_DEV_CONF, DEF_TIM_VAL);
}

static void reset_nor()
{
	struct sfc_transfer transfer;

	memset(&transfer, 0, sizeof(transfer));

	transfer.cmd_info.cmd = SPINOR_OP_RSTEN;
	transfer.cmd_info.dataen = DISABLE;
	transfer.sfc_mode = 0;
	flash->sfc->transfer = &transfer;
	sfc_sync(flash->sfc);

	transfer.cmd_info.cmd = SPINOR_OP_RST;
	transfer.cmd_info.dataen = DISABLE;
	transfer.sfc_mode = 0;
	flash->sfc->transfer = &transfer;
	sfc_sync(flash->sfc);
	udelay(100);
}


void sfc_init()
{
	struct mini_spi_nor_info *spi_nor_info;

	clk_set_rate(SFC, CONFIG_SFC_RATE);
	sfc->threshold = THRESHOLD;
	flash->sfc = sfc;

	reset_nor();

	set_flash_timing();
	sfc_nor_read_params(CONFIG_SPIFLASH_PART_OFFSET + sizeof(struct burner_params), &flash->g_nor_info, sizeof(struct mini_spi_nor_info));
	printf("%s %x\n", flash->g_nor_info.name, flash->g_nor_info.id);

	spi_nor_info = &flash->g_nor_info;

	flash->cur_r_cmd = &spi_nor_info->read_standard;
#ifdef CONFIG_SFC_QUAD
	switch (spi_nor_info->quad_ops_mode) {
	case 0:
		set_quad_mode_cmd();
		break;
	case 1:
		set_quad_mode_reg();
		break;
	default:
		break;
	}
#endif

	if (spi_nor_info->chip_size > 0x1000000) {
		switch (spi_nor_info->addr_ops_mode) {
			case 0:
				enter_4byte();
				break;
			case 1:
				write_enable();
				enter_4byte();
				break;
			default:
				break;
		}
	}
}

static unsigned int sfc_do_read(unsigned int addr, unsigned char *buf, unsigned int len)
{
	unsigned char command;
	int dummy_byte;
	int addr_size;
	int transfer_mode;
	int ret;
	struct sfc_transfer transfer;

	command = flash->cur_r_cmd->cmd;
	dummy_byte = flash->cur_r_cmd->dummy_byte;
	transfer_mode = flash->cur_r_cmd->transfer_mode;
	addr_size = flash->cur_r_cmd->addr_nbyte;

	memset(&transfer, 0, sizeof(transfer));

	transfer.cmd_info.cmd = command;
	transfer.cmd_info.cmd = flash->cur_r_cmd->cmd;
	transfer.cmd_info.dataen = ENABLE;
	transfer.addr_len = addr_size;
	transfer.data_dummy_bits = dummy_byte;
	transfer.addr = addr;
	transfer.len = len;
	transfer.data = buf;
	transfer.cur_len = 0;
	transfer.ops_mode = CPU_OPS;
	transfer.sfc_mode = transfer_mode;
	transfer.direction = GLB_TRAN_DIR_READ;
	flash->sfc->transfer = &transfer;
	sfc_sync(flash->sfc);

	return len;
}

int sfc_read_data(unsigned int from, unsigned int len, unsigned char *buf)
{
	int tmp_len = 0, current_len = 0;

	while(len) {
		tmp_len = sfc_do_read((unsigned int)from + current_len, &buf[current_len], len);
		current_len += tmp_len;
		len -= tmp_len;
	}

	return current_len;

}


#ifdef CONFIG_OTA_VERSION20
static void nv_map_area(unsigned int *base_addr, unsigned int nv_addr, unsigned int blocksize)
{
	unsigned int buf[3][2];
	unsigned int tmp_buf[4];
	unsigned int nv_num = 0, nv_count = 0;
	unsigned int addr, i;

	for(i = 0; i < 3; i++) {
		addr = nv_addr + i * blocksize;
		sfc_read_data(addr, 4, buf[i]);
		if(buf[i][0] == 0x5a5a5a5a) {
			sfc_read_data(addr + 1 *1024,  16, tmp_buf);
			addr += blocksize - 8;
			sfc_read_data(addr, 8, buf[i]);
			if(buf[i][1] == 0xa5a5a5a5) {
				if(nv_count < buf[i][0]) {
					nv_count = buf[i][0];
					nv_num = i;
				}
			}
		}
	}

	*base_addr = nv_addr + nv_num * blocksize;
}
#endif
void spl_sfc_nor_load_image(void)
{
	struct image_header *header;
#ifdef CONFIG_SPL_OS_BOOT
	unsigned int bootimg_addr = 0;
	struct norflash_partitions partition;
	int i;
#ifdef CONFIG_OTA_VERSION20
	unsigned int nv_rw_addr;
	unsigned int nor_blocksize;
	unsigned int src_addr, updata_flag;
	unsigned nv_buf[2];
	int count = 8;
#endif
#endif
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);
	//memset(header, 0, sizeof(struct image_header));
	sfc_init();
#ifdef CONFIG_SPL_OS_BOOT
	sfc_read_data(CONFIG_SPIFLASH_PART_OFFSET + sizeof(struct spi_nor_info) + sizeof(int) * 2, sizeof(struct norflash_partitions), (unsigned int*)&partition);
	for (i = 0 ; i < partition.num_partition_info; i ++) {
		if (!strncmp(partition.nor_partition[i].name, CONFIG_SPL_OS_NAME, sizeof(CONFIG_SPL_OS_NAME))) {
			bootimg_addr = partition.nor_partition[i].offset;
		}
#ifdef CONFIG_OTA_VERSION20
		if (!strncmp(partition.nor_partition[i].name, CONFIG_PAR_NV_NAME, sizeof(CONFIG_PAR_NV_NAME))) {
			nv_rw_addr = partition.nor_partition[i].offset;
			nor_blocksize = partition.nor_partition[i].size / CONFIG_PAR_NV_NUM;
		}
#endif
	}
#ifndef CONFIG_OTA_VERSION20 /* norflash spl boot kernel */
	sfc_read_data(bootimg_addr, sizeof(struct image_header), CONFIG_SYS_TEXT_BASE);
	spl_parse_image_header(header);
	sfc_read_data(bootimg_addr, spl_image.size, spl_image.load_addr);
	return ;
#else //not defined CONFIG_NOR_SPL_BOOT_OS
	nv_map_area((unsigned int)&src_addr, nv_rw_addr, nor_blocksize);
	sfc_read_data(src_addr, count, nv_buf);
	updata_flag = nv_buf[1];
	if((updata_flag & 0x3) != 0x3)
	{
		sfc_read_data(bootimg_addr, sizeof(struct image_header), CONFIG_SYS_TEXT_BASE);
		spl_parse_image_header(header);
		sfc_read_data(bootimg_addr, spl_image.size, spl_image.load_addr);
	} else
#endif	/* CONFIG_OTA_VERSION20 */
#endif	/* CONFIG_SPL_OS_BOOT */
	{
		spl_parse_image_header(header);
		sfc_read_data(CONFIG_UBOOT_OFFSET, CONFIG_SYS_MONITOR_LEN,CONFIG_SYS_TEXT_BASE);
	}
	return ;

}


