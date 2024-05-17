#include <common.h>
#include <config.h>
#include <spl.h>
#include <asm/io.h>
#include <errno.h>
#include <linux/err.h>
#include <malloc.h>
#include <asm/arch/clk.h>
#include <div64.h>

#include <asm/jz_cache.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spinor.h>
#include <generated/sfc_timing_val.h>
#include "spl_rtos.h"
#include "spl_rtos_argument.h"

static struct spl_rtos_argument spl_rtos_args;
static struct rtos_boot_os_args os_boot_args;


#define STATUS_MAX_LEN  4      //4 * byte = 32 bit

//#define SFC_NOR_DEBUG
#ifdef SFC_NOR_DEBUG
#define sfc_debug(fmt, args...)			\
	do {					\
		printf(fmt, ##args);		\
	} while(0)
#else
#define sfc_debug(fmt, args...)			\
	do {					\
	} while(0)
#endif


struct sfc_flash *flash = (struct sfc_flash *)(CONFIG_SYS_TEXT_BASE + 0x500000);
struct sfc *sfc = (struct sfc *)(CONFIG_SYS_TEXT_BASE + 0x504000);
struct spi_nor_cmd_info sector_erase;

#ifdef CONFIG_X2580
static int x2580_sfc_change_io_function(int is_quad)
{
	if (!is_quad)
		return 0;

	/*
	 * 解决X2580 SFC quad模式读写异常
	 * 更改SFC0控制器 CLK/D0~D3 output1-output0-input,再将恢复为func1功能, CE管脚不操作
	 *
	 * PA23 : SFC_DT_IO0
	 * PA24 : SFC_DR_IO1
	 * PA25 : SFC_HOLD_IO4
	 * PA26 : SFC_WP_IO2
	 * PA27 : SFC_CLK
	 * PA28 : SFC_CE
	 *            0x10  0x20  0x30  0x40
	 *            INT   MASK  PAT1  PAT0
	 * func1       0     0     0     1
	 * output0     0     1     0     0
	 */
	gpio_set_func(0, GPIO_OUTPUT1, 0x1f << 23);
	gpio_set_func(0, GPIO_OUTPUT0, 0x1f << 23);
	gpio_set_func(0, GPIO_INPUT, 0x1f << 23);
	gpio_set_func(0, GPIO_FUNC_1, 0x1f << 23);

	return 0;
}

static int ingenic_sfc_gpio_slew_driver_strength(void)
{
	unsigned int base = GPIO_BASE + 0x1000 * 0;
	/*
	 * SFC: PA23 ~ PA28
	 * Slew : 0x10010160  ===> 1 : Fast mode
	 * Driver strength
	 * DS1 DS0
	 *  0   0    ===> 2mA
	 *  0   1    ===> 4mA
	 *  1   0    ===> 8mA
	 *  1   1    ===> 12mA
	 */
	writel(0x1F800000, base + PXPSLWS);  /* Slew===> 1 */
	writel(0x1F800000, base + PXPDS0S);  /* DS0 ===> 1 */
	writel(0x1F800000, base + PXPDS1C);  /* DS1 ===> 0 */
}
#endif

/* Prevent cur_r_cmd from being overwritten when the firmware is too large */
unsigned int cur_r_cmd;

#ifdef SFC_NOR_DEBUG
void dump_cdt(struct sfc *sfc)
{
	struct sfc_cdt *cdt;
	int i;

	if(sfc->cdt_addr == NULL){
		sfc_debug("%s error: sfc res not init !\n", __func__);
		return;
	}

	cdt = sfc->cdt_addr;

	for(i = 0; i < 32; i++){
		sfc_debug("\nnum------->%d\n", i);
		sfc_debug("link:%x, ENDIAN:%x, WORD_UINT:%x, TRAN_MODE:%x, ADDR_KIND:%x\n",
				(cdt[i].link >> 31) & 0x1, (cdt[i].link >> 18) & 0x1,
				(cdt[i].link >> 16) & 0x3, (cdt[i].link >> 4) & 0xf,
				(cdt[i].link >> 0) & 0x3
				);
		sfc_debug("CLK_MODE:%x, ADDR_WIDTH:%x, POLL_EN:%x, CMD_EN:%x,PHASE_FORMAT:%x, DMY_BITS:%x, DATA_EN:%x, TRAN_CMD:%x\n",
				(cdt[i].xfer >> 29) & 0x7, (cdt[i].xfer >> 26) & 0x7,
				(cdt[i].xfer >> 25) & 0x1, (cdt[i].xfer >> 24) & 0x1,
				(cdt[i].xfer >> 23) & 0x1, (cdt[i].xfer >> 17) & 0x3f,
				(cdt[i].xfer >> 16) & 0x1, (cdt[i].xfer >> 0) & 0xffff
				);
		sfc_debug("DEV_STA_EXP:%x\n", cdt[i].staExp);
		sfc_debug("DEV_STA_MSK:%x\n", cdt[i].staMsk);
	}
}
#endif

static inline void sfc_writel(unsigned short offset, u32 value)
{
	writel(value, SFC_BASE + offset);
}

static inline unsigned int sfc_readl(unsigned short offset)
{
	return readl(SFC_BASE + offset);
}

static inline void sfc_flush_and_start(void)
{
	sfc_writel(SFC_TRIG, TRIG_FLUSH);
	sfc_writel(SFC_TRIG, TRIG_START);
}

static inline void sfc_clear_all_intc(void)
{
	sfc_writel(SFC_SCR, 0x1f);
}

static inline void sfc_mask_all_intc()
{
	sfc_writel(SFC_INTC, 0x1f);
}

static inline void sfc_set_mem_addr(unsigned int addr)
{
	sfc_writel(SFC_MEM_ADDR, addr);
}

static inline void sfc_set_length(int value)
{
	sfc_writel(SFC_TRAN_LEN, value);
}

static inline unsigned int sfc_read_rxfifo(void)
{
	return sfc_readl(SFC_RM_DR);
}

static inline void sfc_write_txfifo(const unsigned int value)
{
	sfc_writel(SFC_RM_DR, value);
}

static inline unsigned int get_sfc_ctl_sr(void)
{
	return sfc_readl(SFC_SR);
}

static unsigned int cpu_read_rxfifo(struct sfc_cdt_xfer *xfer)
{
	int i;
	unsigned long align_len = 0;
	unsigned int fifo_num = 0;

	align_len = ALIGN(xfer->config.datalen, 4);

	if (((align_len - xfer->config.cur_len) / 4) > THRESHOLD) {
		fifo_num = THRESHOLD;
	} else {
		fifo_num = (align_len - xfer->config.cur_len) / 4;
	}

	for (i = 0; i < fifo_num; i++) {
		*(unsigned int *)xfer->config.buf = sfc_read_rxfifo();
		xfer->config.buf += 4;
		xfer->config.cur_len += 4;
	}

	return 0;
}

static void cpu_write_txfifo(struct sfc_cdt_xfer *xfer)
{
	/**
	 * Assuming that all data is less than one word,
	 * if len large than one word, unsupport!
	 **/

	sfc_write_txfifo(*(unsigned int *)xfer->config.buf);
}

static void sfc_sr_handle(struct sfc_cdt_xfer *xfer)
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
			cpu_read_rxfifo(xfer);
		}

		if (reg_sr & CLR_TREQ) {
			sfc_writel(SFC_SCR, CLR_TREQ);
			cpu_write_txfifo(xfer);
		}

		if (reg_sr & CLR_UNDER) {
			tmp = CLR_UNDER;
			sfc_debug("UNDR!\n");
			break;
		}

		if (reg_sr & CLR_OVER) {
			tmp = CLR_OVER;
			sfc_debug("OVER!\n");
			break;
		}
	}
	if (tmp)
		sfc_writel(SFC_SCR, tmp);
}

static void sfc_start_transfer(struct sfc_cdt_xfer *xfer)
{
	sfc_clear_all_intc();
	sfc_mask_all_intc();
	sfc_flush_and_start();

	sfc_sr_handle(xfer);

}

static void sfc_use_cdt(void)
{
	uint32_t tmp = sfc_readl(SFC_GLB);
	tmp |= GLB_CDT_EN;
	sfc_writel(SFC_GLB, tmp);
}

static void write_cdt(struct sfc *sfc, struct sfc_cdt *cdt, uint16_t start_index, uint16_t end_index)
{
	uint32_t cdt_num, cdt_size;

	cdt_num = end_index - start_index + 1;
	cdt_size = sizeof(struct sfc_cdt);

	memcpy((void *)sfc->cdt_addr + (start_index * cdt_size), (void *)cdt + (start_index * cdt_size), cdt_num * cdt_size);
	sfc_debug("create CDT index: %d ~ %d,  index number:%d.\n", start_index, end_index, cdt_num);
}

static void sfc_set_index(unsigned short index)
{

	uint32_t tmp = sfc_readl(SFC_CMD_IDX);
	tmp &= ~CMD_IDX_MSK;
	tmp |= index;
	sfc_writel(SFC_CMD_IDX, tmp);
}

static void sfc_set_dataen(uint8_t dataen)
{

	uint32_t tmp = sfc_readl(SFC_CMD_IDX);
	tmp &= ~CDT_DATAEN_MSK;
	tmp |= (dataen << CDT_DATAEN_OFF);
	sfc_writel(SFC_CMD_IDX, tmp);
}

static void sfc_set_datadir(uint8_t datadir)
{

	uint32_t tmp = sfc_readl(SFC_CMD_IDX);
	tmp &= ~CDT_DIR_MSK;
	tmp |= (datadir << CDT_DIR_OFF);
	sfc_writel(SFC_CMD_IDX, tmp);
}

static void sfc_set_addr(struct sfc_cdt_xfer *xfer)
{
	sfc_writel(SFC_COL_ADDR, xfer->columnaddr);
	sfc_writel(SFC_ROW_ADDR, xfer->rowaddr);
	sfc_writel(SFC_STA_ADDR0, xfer->staaddr0);
	sfc_writel(SFC_STA_ADDR1, xfer->staaddr1);
}

static void sfc_transfer_mode(int value)
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_GLB);
	if (value == CPU_OPS)
		tmp &= ~GLB_OP_MODE;
	else
		tmp |= GLB_OP_MODE;
	sfc_writel(SFC_GLB, tmp);
}

static void sfc_set_data_config(struct sfc_cdt_xfer *xfer)
{
	sfc_set_dataen(xfer->dataen);

	sfc_set_length(0);
	if(xfer->dataen){
		sfc_set_datadir(xfer->config.data_dir);
		sfc_transfer_mode(xfer->config.ops_mode);
		sfc_set_length(xfer->config.datalen);

		/* default use cpu mode */
		sfc_set_mem_addr(0);
	}
}

static void sfc_sync_cdt(struct sfc_cdt_xfer *xfer)
{
	/* 1. set cmd index */
	sfc_set_index(xfer->cmd_index);

	/* 2. set addr */
	sfc_set_addr(xfer);

	/* 3. config data config */
	sfc_set_data_config(xfer);

	/* 4. start transfer */
	sfc_start_transfer(xfer);
}

void sfc_threshold(struct sfc *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_GLB);
	tmp &= ~GLB_THRESHOLD_MSK;
	tmp |= sfc->threshold << GLB_THRESHOLD_OFFSET;
	sfc_writel(SFC_GLB, tmp);
}

static void write_enable(void)
{
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	/* set index */
	xfer.cmd_index = NOR_WRITE_ENABLE;

	/* set addr */
	xfer.columnaddr = 0;

	/* set transfer config */
	xfer.dataen = DISABLE;

	sfc_sync_cdt(&xfer);
}

static void enter_4byte(void)
{
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	/* set index */
	xfer.cmd_index = NOR_EN_4BYTE;

	/* set addr */
	xfer.columnaddr = 0;

	/* set transfer config */
	xfer.dataen = DISABLE;

	sfc_sync_cdt(&xfer);
}

static void inline set_quad_mode_cmd(void)
{
	flash->cur_r_cmd = NOR_READ_QUAD;
}

/* write nor flash status register QE bit to set quad mode */
static void set_quad_mode_reg(void)
{
	struct mini_spi_nor_info *spi_nor_info;
	struct spi_nor_st_info *quad_set;

	struct sfc_cdt_xfer xfer;
	unsigned int data;

	spi_nor_info = &flash->g_nor_info;
	quad_set = &spi_nor_info->quad_set;
	data = (quad_set->val & quad_set->mask) << quad_set->bit_shift;

	/* 1. set nor quad */
	memset(&xfer, 0, sizeof(xfer));
	/* set index */
	xfer.cmd_index = NOR_QUAD_SET_ENABLE;

	/* set addr */
	xfer.columnaddr = 0;
	xfer.rowaddr = 0;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = quad_set->len;
	xfer.config.data_dir = GLB_TRAN_DIR_WRITE;
	xfer.config.ops_mode = CPU_OPS;
	xfer.config.buf = (uint8_t *)&data;

	sfc_sync_cdt(&xfer);

	flash->cur_r_cmd = NOR_READ_QUAD;
}

static void sfc_nor_read_params(unsigned int addr, unsigned char *buf, unsigned int len)
{
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	xfer.cmd_index = NOR_READ_STANDARD;

	/* set addr */
	xfer.columnaddr = 0;
	xfer.rowaddr = addr;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = len;
	xfer.config.data_dir = GLB_TRAN_DIR_READ;
	xfer.config.ops_mode = CPU_OPS;
	xfer.config.buf = buf;

	sfc_sync_cdt(&xfer);
}

static inline void set_flash_timing(void)
{
	sfc_writel(SFC_DEV_CONF, DEF_TIM_VAL);
}

static void reset_nor(void)
{
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	xfer.cmd_index = NOR_RESET_ENABLE;

	/* set addr */
	xfer.rowaddr = 0;
	xfer.columnaddr = 0;

	/* set transfer config */
	xfer.dataen = DISABLE;

	sfc_sync_cdt(&xfer);

	udelay(100);
}

static void params_to_cdt(struct mini_spi_nor_info *params, struct sfc_cdt *cdt)
{
	/* 4.nor singleRead */
	MK_CMD(cdt[NOR_READ_STANDARD], params->read_standard, 0, ROW_ADDR, ENABLE);

	/* 5.nor quadRead */
	MK_CMD(cdt[NOR_READ_QUAD], params->read_quad, 0, ROW_ADDR, ENABLE);

#if 0
	/* 6. nor writeStandard */
	MK_CMD(cdt[NOR_WRITE_STANDARD_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_WRITE_STANDARD], params->write_standard, 1, ROW_ADDR, ENABLE);
	MK_ST(cdt[NOR_WRITE_STANDARD_FINISH], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);

	/* 7. nor writeQuad */
	MK_CMD(cdt[NOR_WRITE_QUAD_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_WRITE_QUAD], params->write_quad, 1, ROW_ADDR, ENABLE);
	MK_ST(cdt[NOR_WRITE_QUAD_FINISH], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);
#endif
	/* 8. nor erase */
	MK_CMD(cdt[NOR_ERASE_WRITE_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
	MK_CMD(cdt[NOR_ERASE], sector_erase, 1, ROW_ADDR, DISABLE);
	MK_ST(cdt[NOR_ERASE_FINISH], params->busy, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);



	/* 9. quad mode */
	if(params->quad_ops_mode){
		MK_CMD(cdt[NOR_QUAD_SET_ENABLE], params->wr_en, 1, DEFAULT_ADDRMODE, DISABLE);
		MK_ST(cdt[NOR_QUAD_SET], params->quad_set, 1, DEFAULT_ADDRMODE, 0, DISABLE, ENABLE, TM_STD_SPI);  //disable poll, enable data
		MK_ST(cdt[NOR_QUAD_FINISH], params->busy, 1, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);
		MK_ST(cdt[NOR_QUAD_GET], params->quad_get, 0, DEFAULT_ADDRMODE, 0, ENABLE, DISABLE, TM_STD_SPI);
	}

	/* 10. nor write ENABLE */
	MK_CMD(cdt[NOR_WRITE_ENABLE], params->wr_en, 0, DEFAULT_ADDRMODE, DISABLE);

	/* 11. entry 4byte mode */
	MK_CMD(cdt[NOR_EN_4BYTE], params->en4byte, 0, DEFAULT_ADDRMODE, DISABLE);

}

static void create_cdt_table(struct sfc_flash *flash, uint32_t flag)
{
	struct mini_spi_nor_info *nor_flash_info;
	struct sfc_cdt cdt[INDEX_MAX_NUM];

	memset(cdt, 0, sizeof(cdt));

	/* 1.nor reset */
	cdt[NOR_RESET_ENABLE].link = CMD_LINK(1, DEFAULT_ADDRMODE, TM_STD_SPI);
	cdt[NOR_RESET_ENABLE].xfer = CMD_XFER(0, DISABLE, 0, DISABLE, SPINOR_OP_RSTEN);
	cdt[NOR_RESET_ENABLE].staExp = 0;
	cdt[NOR_RESET_ENABLE].staMsk = 0;

	cdt[NOR_RESET].link = CMD_LINK(0, DEFAULT_ADDRMODE, TM_STD_SPI);
	cdt[NOR_RESET].xfer = CMD_XFER(0, DISABLE, 0, DISABLE, SPINOR_OP_RST);
	cdt[NOR_RESET].staExp = 0;
	cdt[NOR_RESET].staMsk = 0;

	/* 2.nor read id */
	cdt[NOR_READ_ID].link = CMD_LINK(0, DEFAULT_ADDRMODE, TM_STD_SPI);
	cdt[NOR_READ_ID].xfer = CMD_XFER(0, DISABLE, 0, ENABLE, SPINOR_OP_RDID);
	cdt[NOR_READ_ID].staExp = 0;
	cdt[NOR_READ_ID].staMsk = 0;

	/* 3. nor get status */
	cdt[NOR_GET_STATUS].link = CMD_LINK(0, DEFAULT_ADDRMODE, TM_STD_SPI);
	cdt[NOR_GET_STATUS].xfer = CMD_XFER(0, DISABLE, 0, ENABLE, SPINOR_OP_RDSR);
	cdt[NOR_GET_STATUS].staExp = 0;
	cdt[NOR_GET_STATUS].staMsk = 0;

	cdt[NOR_GET_STATUS_1].link = CMD_LINK(0, DEFAULT_ADDRMODE, TM_STD_SPI);
	cdt[NOR_GET_STATUS_1].xfer = CMD_XFER(0, DISABLE, 0, ENABLE, SPINOR_OP_RDSR_1);
	cdt[NOR_GET_STATUS_1].staExp = 0;
	cdt[NOR_GET_STATUS_1].staMsk = 0;

	cdt[NOR_GET_STATUS_2].link = CMD_LINK(0, DEFAULT_ADDRMODE, TM_STD_SPI);
	cdt[NOR_GET_STATUS_2].xfer = CMD_XFER(0, DISABLE, 0, ENABLE, SPINOR_OP_RDSR_2);
	cdt[NOR_GET_STATUS_2].staExp = 0;
	cdt[NOR_GET_STATUS_2].staMsk = 0;

	if(flag == DEFAULT_CDT){
		/* 4.nor singleRead */
		cdt[NOR_READ_STANDARD].link = CMD_LINK(0, ROW_ADDR, TM_STD_SPI);
		cdt[NOR_READ_STANDARD].xfer = CMD_XFER(DEFAULT_ADDRSIZE, DISABLE, 0, ENABLE, SPINOR_OP_READ);
		cdt[NOR_READ_STANDARD].staExp = 0;
		cdt[NOR_READ_STANDARD].staMsk = 0;

		/* first create cdt table */
		write_cdt(flash->sfc, cdt, NOR_RESET_ENABLE, NOR_READ_STANDARD);
	}

	if(flag == UPDATE_CDT){
		nor_flash_info = &flash->g_nor_info;
		params_to_cdt(nor_flash_info, cdt);
		write_cdt(flash->sfc, cdt, NOR_READ_STANDARD, NOR_EN_4BYTE);
	}
#ifdef SFC_NOR_DEBUG
	dump_cdt(flash->sfc);
#endif
}

unsigned int sfc_nor_read_id(void)
{
	struct sfc_cdt_xfer xfer;
	unsigned char buf[3];
	unsigned int chip_id = 0;

	memset(&xfer, 0, sizeof(xfer));

	/* set Index */
	xfer.cmd_index = NOR_READ_ID;

	/* set addr */
	xfer.rowaddr = 0;
	xfer.columnaddr = 0;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = 3;
	xfer.config.data_dir = GLB_TRAN_DIR_READ;
	xfer.config.ops_mode = CPU_OPS;
	xfer.config.buf = buf;

	sfc_sync_cdt(&xfer);

	chip_id = ((buf[0] & 0xff) << 16) | ((buf[1] & 0xff) << 8) | (buf[2] & 0xff);
	return chip_id;
}

unsigned int get_part_offset_by_name(struct norflash_partitions partition, char *name)
{
	int i = 0;

	for (i = 0; i < partition.num_partition_info; i++) {
		if (!strcmp(partition.nor_partition[i].name, name)) {
			return partition.nor_partition[i].offset;
		}
	}

	return -1;
}

unsigned int get_part_size_by_name(struct norflash_partitions partition, char *name)
{
	int i = 0;

	for (i = 0; i < partition.num_partition_info; i++) {
		if (!strcmp(partition.nor_partition[i].name, name)) {
			return partition.nor_partition[i].size;
		}
	}

	return -1;
}

#ifdef CONFIG_JZ_SECURE_SUPPORT
extern int secure_scboot (void *, void *);
extern int is_security_boot(void);

#ifdef CONFIG_JZ_SECURE_ROOTFS
#define LOAD_ROOTFS_ADDR 0x82000000
static void secure_check_hash_rootfs(struct norflash_partitions partitions)
{
	unsigned int signature_offset;
	unsigned int rootfs_offset;
	unsigned int code_len;
	unsigned int *ptr = (unsigned int *)(LOAD_ROOTFS_ADDR - 2048);
	int ret;

	signature_offset = get_part_offset_by_name(partitions, CONFIG_SPL_SIG_NAME);
	rootfs_offset = get_part_offset_by_name(partitions, CONFIG_SPL_ROOTFS_NAME);

	if (signature_offset == -1 || rootfs_offset == -1){
		printf("sig or rootfs part not found\n");
		hang();
	}

	sfc_read_data(signature_offset, 2048, LOAD_ROOTFS_ADDR - 2048);
	code_len = ptr[128];

	sfc_read_data(rootfs_offset, code_len, LOAD_ROOTFS_ADDR);

	ret = secure_scboot(LOAD_ROOTFS_ADDR - 2048, LOAD_ROOTFS_ADDR);
	if(ret) {
		printf("Error check rootfs hash\n");
		hang();
	}
}
#endif
#endif

void spl_load_kernel(long offset)
{
	struct image_header *header;
	struct image_info info;
	int ret;
	void *load_buf, *image_buf;
	int image_len;
#ifdef CONFIG_JZ_SECURE_SUPPORT
	header = (struct image_header *)(CONFIG_SYS_SC_TEXT_BASE);

	sfc_read_data(offset, sizeof(struct image_header) + sizeof(int), (unsigned char *)CONFIG_SYS_TEXT_BASE);
	header->ih_name[IH_NMLEN - 1] = 0;

	spl_parse_image_header(header);
	spl_image.load_addr -= 2048;
	sfc_read_data(offset, spl_image.size, (unsigned char *)spl_image.load_addr);

	flush_scache_range(spl_image.load_addr, spl_image.load_addr + spl_image.size);

	ret = secure_scboot(spl_image.load_addr, spl_image.load_addr + 2048);
	if(ret) {
		printf("Error spl secure load kernel\n");
		hang();
	}
#else
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

	sfc_read_data(offset, sizeof(struct image_header), (unsigned char *)CONFIG_SYS_TEXT_BASE);
	header->ih_name[IH_NMLEN - 1] = 0;
	spl_parse_image_header(header);
#ifdef CONFIG_JZ_HARDLZMA
	sfc_read_data(offset, spl_image.size, (unsigned char *)CONFIG_SYS_TEXT_BASE);

	spl_parse_image_info(header, &info);

	load_buf = map_sysmem(info.load, info.image_len);
	image_buf = map_sysmem(info.image_start, info.image_len);
	image_len = info.image_len;

	if (info.comp == IH_COMP_HARDLZMA) {
		printf("Uncompressing LZMA Hardware ... \n");
/*lzma 硬件解压*/
		flush_cache_all();
		ret = jz_lzma_decompress(image_buf, image_len, load_buf, CONFIG_HARD_LZMA_CHANNEL);
		flush_cache_all();
		if(ret <= 0) {
			printf("lzam hardware decompress uImage failed \n");
			hang();
		}
 	} else
		printf("The kernel compression type is incorrect\n");
#else
	sfc_read_data(offset, spl_image.size, (unsigned char *)spl_image.load_addr);
#endif
#endif
}

#ifdef CONFIG_SPL_EXTRA_NOR_INFO_ENABLE

#ifndef CONFIG_SPL_EXTRA_NOR_INFO_OFF
#define CONFIG_SPL_EXTRA_NOR_INFO_OFF CONFIG_SPL_PAD_TO
#endif

struct spi_nor_info_tag {
    char tag[8];
    int array_size;
};

void copy_to_mini_info(struct spi_nor_info *s, struct mini_spi_nor_info *m)
{
    memcpy(m->name, s->name, sizeof(m->name));
	m->id = s->id;
	m->read_standard = s->read_standard;
	m->read_quad = s->read_quad;
	m->wr_en = s->wr_en;
	m->en4byte = s->en4byte;
	m->quad_set = s->quad_set;
	m->quad_get = s->quad_get;
	m->busy = s->busy;
	m->quad_ops_mode = s->quad_ops_mode;
	m->addr_ops_mode = s->addr_ops_mode;
	m->chip_size = s->chip_size;
	m->page_size = s->page_size;
	m->erase_size = s->erase_size;
}

#endif

void sfc_init(void)
{
	unsigned int erase_cmd_offset;
	struct mini_spi_nor_info *spi_nor_info;
#ifdef CONFIG_SFC_NOR_INIT_RATE
	clk_set_rate(SFC, CONFIG_SFC_NOR_INIT_RATE);
#else
	/* default: sfc rate 50MHz */
	clk_set_rate(SFC, 200000000L);
#endif

	sfc->threshold = THRESHOLD;
	flash->sfc = sfc;

	/* use CDT mode */
	sfc_use_cdt();
	sfc_debug("Enter 'CDT' mode.\n");

	/* try creating default CDT table */
	flash->sfc->cdt_addr = (volatile void *)(SFC_BASE + SFC_CDT);
	create_cdt_table(flash, DEFAULT_CDT);

	/* reset nor flash */
	reset_nor();

#ifdef CONFIG_X2580
	ingenic_sfc_gpio_slew_driver_strength();
#endif

	/* config sfc */
	set_flash_timing();
	sfc_threshold(flash->sfc);

	unsigned int nor_id = sfc_nor_read_id();
	/* get nor flash params */
	erase_cmd_offset = offsetof(struct burner_params, spi_nor_info.sector_erase);
	sfc_nor_read_params(CONFIG_SPIFLASH_PART_OFFSET + sizeof(struct burner_params), (unsigned char *)&flash->g_nor_info, sizeof(struct mini_spi_nor_info));
	sfc_nor_read_params(CONFIG_SPIFLASH_PART_OFFSET + erase_cmd_offset, (unsigned char *)&sector_erase, sizeof(struct spi_nor_cmd_info));
#ifdef CONFIG_SPL_EXTRA_NOR_INFO_ENABLE
	if (nor_id != flash->g_nor_info.id) {
		struct spi_nor_info_tag tag;
		sfc_nor_read_params(CONFIG_SPL_EXTRA_NOR_INFO_OFF, (void *)&tag, sizeof(tag));
		if (!strncmp(tag.tag, "nor_tag", sizeof(tag.tag))) {
			struct spi_nor_info info;
			int i;
			for (i = 0; i < tag.array_size; i++) {
				int off = CONFIG_SPL_EXTRA_NOR_INFO_OFF + sizeof(tag) + i*sizeof(info);
				sfc_nor_read_params(off, (void *)&info, sizeof(info));
				if (nor_id == info.id) {
				  	copy_to_mini_info(&info, &flash->g_nor_info);
					break;
				}
			}
			if (i == tag.array_size)
				printf("not match extra nor info: %x\n", nor_id);
		} else {
			printf("not found extra nor info array\n");
		}
	}
#endif

	printf("%s %x %x\n", flash->g_nor_info.name, flash->g_nor_info.id, nor_id);

	/* update to private CDT table */
	create_cdt_table(flash, UPDATE_CDT);

	spi_nor_info = &flash->g_nor_info;

	flash->cur_r_cmd = NOR_READ_STANDARD;

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
	cur_r_cmd = flash->cur_r_cmd;
}

static void sfc_do_erase_blk(unsigned int addr)
{
    struct sfc_cdt_xfer xfer;
    memset(&xfer, 0, sizeof(xfer));

    /* set Index */
    xfer.cmd_index = NOR_ERASE_WRITE_ENABLE;

    /* set addr */
    xfer.rowaddr = addr;

    /* set transfer config */
    xfer.dataen = DISABLE;

    sfc_sync_cdt(&xfer);
}

void sfc_erase_data(unsigned int addr, unsigned int len)
{
    unsigned int end;
    unsigned int erasesize = flash->g_nor_info.erase_size;

    if ((erasesize-1) & addr) {
        printf("erase error: address isn't aligned with block_size\n");
        hang();
    }

    if ((erasesize-1) & len) {
        printf("erase error: len must be times of blocks_size\n");
        hang();
	}

    end = addr + len;
    while (addr < end) {
        sfc_do_erase_blk(addr);
        addr += erasesize;
    }
}

static unsigned int sfc_do_read(unsigned int addr, unsigned char *buf, unsigned int len)
{
	struct sfc_cdt_xfer xfer;
	memset(&xfer, 0, sizeof(xfer));

#ifdef CONFIG_X2580
	if (cur_r_cmd == NOR_READ_QUAD)
		x2580_sfc_change_io_function(1);
#endif

	/* set Index */
	xfer.cmd_index = cur_r_cmd;

	/* set addr */
	xfer.columnaddr = 0;
	xfer.rowaddr = addr;

	/* set transfer config */
	xfer.dataen = ENABLE;
	xfer.config.datalen = len;
	xfer.config.data_dir = GLB_TRAN_DIR_READ;
	xfer.config.ops_mode = CPU_OPS;
	xfer.config.buf = buf;

	sfc_sync_cdt(&xfer);

	return len;
}

int sfc_read_data(unsigned int from, unsigned int len, unsigned char *buf)
{
	int tmp_len = 0, current_len = 0;

	while((int)len > 0) {
		tmp_len = sfc_do_read((unsigned int)from + current_len, &buf[current_len], len);
		current_len += tmp_len;
		len -= tmp_len;
	}

	return current_len;

}


#ifdef CONFIG_OTA_VERSION20
static void nv_map_area(unsigned int *base_addr, unsigned int nv_addr, unsigned int nv_size)
{
	unsigned int buf[6][2];
	unsigned int tmp_buf[4];
	unsigned int nv_off = 0, nv_count = 0;
	unsigned int addr, i;
	unsigned int blocksize = flash->g_nor_info.erase_size;
	unsigned int nv_num = nv_size / blocksize;

	if(nv_num > 6) {
	//	sfc_debug("%s,bigger\n",__func__);
		while(1);
	}

	for(i = 0; i < nv_num; i++) {
		addr = nv_addr + i * blocksize;
		sfc_read_data(addr, 4, (unsigned char *)buf[i]);
		if(buf[i][0] == 0x5a5a5a5a) {
			sfc_read_data(addr + 1 *1024,  16, (unsigned char *)tmp_buf);
			addr += blocksize - 8;
			sfc_read_data(addr, 8, (unsigned char *)buf[i]);
			if(buf[i][1] == 0xa5a5a5a5) {
				if(nv_count < buf[i][0]) {
					nv_count = buf[i][0];
					nv_off = i;
				}
			}
		}
	}

	*base_addr = nv_addr + nv_off * blocksize;
}
#endif

#if defined(CONFIG_SPL_RTOS_BOOT) || defined(CONFIG_SPL_RTOS_LOAD_KERNEL) || defined(CONFIG_BOOT_RTOS_OTA)

struct rtos_header rtos_header;

#ifdef CONFIG_RTOS_BOOT_ON_SECOND_CPU
#include <asm/arch/ccu.h>

unsigned char second_cpu_little_stack[128] __attribute__((aligned(8)));

__attribute__ ((noreturn)) void do_boot_second_cpu(void)
{
	rtos_raw_start(&rtos_header, NULL);
	while (1);
}

static void boot_second_cpu(void)
{
  asm volatile (
    "    .set    push                        \n"
    "    .set    reorder                     \n"
    "    .set    noat                        \n"
    "    la    $29, (second_cpu_little_stack+128)   \n"
    "    j do_boot_second_cpu   \n"
    "    nop                    \n"
    "    .set    pop                         \n"
    :
    :
    : "memory"
    );
}

static void start_second_cpu(void)
{
	unsigned long value;
	volatile unsigned long *rtos_start = (unsigned long *)rtos_header.img_start;

	value = *rtos_start;

	writel((unsigned long)boot_second_cpu, CCU_IO_BASE+CCU_RER);
	writel(0, CCU_IO_BASE+CCU_CSRR);

	if (rtos_header.version & (1 << 0)) {
		while(*rtos_start == value) {
			mdelay(1);
		}
		writel(1 << 1, CCU_IO_BASE+CCU_CSRR);
	}
}
#endif

static int spl_sfc_nor_rtos_load(struct rtos_header *rtos, unsigned int offset)
{
	sfc_read_data(offset, sizeof(*rtos), (unsigned char *)rtos);
	if (rtos_check_header(rtos))
		return -1;

	int size = rtos->img_end - rtos->img_start;
	debug("size = %d tag = 0x%x 0x%x\n",size,rtos->tag,offset);
#ifdef CONFIG_JZ_SCBOOT
	int start = rtos->img_end + 4096;
	sfc_read_data(offset, size, start);
	int ret = secure_scboot((void *)(start + sizeof(struct rtos_header)), (void*)rtos->img_start);
	if(ret) {
		printf("Error spl secure load freertos\n");
		return -1;
	}
#else
	sfc_read_data(offset, size, (unsigned char *)rtos->img_start);
#endif

	return 0;
}

#endif

#ifdef CONFIG_SPL_RTOS_BOOT
static void spl_sfc_nor_rtos_boot(void)
{
	unsigned int rtos_addr = CONFIG_RTOS_OFFSET;
	struct norflash_partitions partition;

	sfc_read_data(CONFIG_SPIFLASH_PART_OFFSET + sizeof(struct spi_nor_info) + sizeof(int) * 2, sizeof(struct norflash_partitions), (unsigned char *)&partition);

#ifdef CONFIG_SPL_OS_OTA_BOOT
	const char *rtos_name = CONFIG_SPL_RTOS_NAME;

	rtos_addr = get_part_offset_by_name(partition, CONFIG_SPL_OTA_NAME);
	if (rtos_addr != -1) {
		char buf[128];
		const char *kernel2 = "ota:"CONFIG_SPL_OS_NAME2;
		sfc_read_data(rtos_addr, sizeof(buf), (unsigned char *)buf);
		if (!strncmp(kernel2, buf, strlen(kernel2))) {
			rtos_name = CONFIG_SPL_RTOS_NAME2;
		}
	}

	rtos_addr = get_part_offset_by_name(partition, rtos_name);
	if (rtos_addr == -1) {
		printf("rtos not found: "CONFIG_SPL_RTOS_NAME"\n");
		hang();
	}

	debug("rtos:%s %x\n", rtos_name, rtos_addr);
#else
    #ifdef CONFIG_SPL_RTOS_NAME
	rtos_addr = get_part_offset_by_name(partition, CONFIG_SPL_RTOS_NAME);
	if (rtos_addr == -1) {
		printf("rtos not found: "CONFIG_SPL_RTOS_NAME"\n");
		printf("use rtos default offset_addr:%d\n", CONFIG_RTOS_OFFSET);
		rtos_addr = CONFIG_RTOS_OFFSET;
	}
    #else
    rtos_addr = CONFIG_RTOS_OFFSET;
    #endif
#endif

	if (spl_sfc_nor_rtos_load(&rtos_header, rtos_addr))
		hang();

	flush_cache_all();

#ifdef CONFIG_RTOS_BOOT_ON_SECOND_CPU
	start_second_cpu();
#else
	/* NOTE: not return */
	rtos_raw_start(&rtos_header, NULL);
#endif
}

#endif

#ifdef CONFIG_SPL_RTOS_LOAD_KERNEL

static void spl_sfc_nor_cfg_os_args(struct norflash_partitions partitions, char *kernel_name, char *cmdargs)
{
	unsigned int img_addr = 0;
	img_addr = get_part_offset_by_name(partitions, kernel_name);
	if (img_addr == -1) {
		printf("kernel not found: "CONFIG_SPL_OS_NAME"\n");
		hang();
	}
	debug("kernel:%s %x\n", kernel_name, img_addr);

	struct image_header *header;

#ifdef CONFIG_JZ_SECURE_SUPPORT
	header = (struct image_header *)(CONFIG_SYS_SC_TEXT_BASE);
	sfc_read_data(img_addr, sizeof(struct image_header) + sizeof(int), (unsigned char *)CONFIG_SYS_TEXT_BASE);
#else
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);
	sfc_read_data(img_addr, sizeof(struct image_header), (unsigned char *)CONFIG_SYS_TEXT_BASE);
#endif

	header->ih_name[IH_NMLEN - 1] = 0;
	spl_parse_image_header(header);

	cmdargs = cmdargs ? cmdargs : CONFIG_SYS_SPL_ARGS_ADDR;
#ifdef CONFIG_SPL_AUTO_PROBE_ARGS_MEM
	cmdargs = spl_board_process_mem_bootargs(cmdargs);
#endif

	/* 由RTOS 加载OS镜像, SPL等待OS加载完成, 并由SPL完成后续引导 */
	os_boot_args.magic = 0x53475241;  /* ARGS */
	os_boot_args.offset = img_addr;
	os_boot_args.size = spl_image.size;
	os_boot_args.cmdargs = cmdargs;
	os_boot_args.entry_point = spl_image.entry_point;

#ifdef CONFIG_JZ_SECURE_SUPPORT
	os_boot_args.load_addr = spl_image.load_addr - 2048;
#else
	os_boot_args.load_addr = spl_image.load_addr;
#endif

	spl_rtos_args.os_boot_args = &os_boot_args;
}

/* not support rtos boot on second cpu */
static char *spl_sfc_nor_boot_rtos_load_os(void)
{
	struct norflash_partitions partitions;
	const char *kernel_name = CONFIG_SPL_OS_NAME;
	const char *rtos_name = CONFIG_SPL_RTOS_NAME;
	char *cmdargs = CONFIG_SYS_SPL_ARGS_ADDR;

	sfc_read_data(CONFIG_SPIFLASH_PART_OFFSET + sizeof(struct spi_nor_info) + sizeof(int) * 2, sizeof(struct norflash_partitions), (unsigned char *)&partitions);

#ifdef CONFIG_SPL_OS_OTA_BOOT
	unsigned int ota_addr = 0;
	ota_addr = get_part_offset_by_name(partitions, CONFIG_SPL_OTA_NAME);
	if (ota_addr != -1) {
		char buf[128];
		const char *kernel2 = "ota:"CONFIG_SPL_OS_NAME2;
		sfc_read_data(ota_addr, sizeof(buf), (unsigned char *)buf);
		if (!strncmp(kernel2, buf, strlen(kernel2))) {
			kernel_name = CONFIG_SPL_OS_NAME2;
			rtos_name = CONFIG_SPL_RTOS_NAME2;
			cmdargs = CONFIG_SYS_SPL_ARGS_ADDR2;
		}
	}
#endif

#ifdef CONFIG_JZ_SECURE_ROOTFS
	secure_check_hash_rootfs(partitions);
#endif
	spl_sfc_nor_cfg_os_args(partitions, kernel_name, cmdargs);

	unsigned int rtos_offset = 0;
	rtos_offset = get_part_offset_by_name(partitions, rtos_name);
	if (rtos_offset == -1) {
		debug("rtos not found: "CONFIG_SPL_RTOS_NAME"\n");
		printf("use rtos default offset_addr:%d\n", CONFIG_RTOS_OFFSET);
		rtos_offset = CONFIG_RTOS_OFFSET;
	}
	debug("rtos:%s %x\n", rtos_name, rtos_offset);

	if (spl_sfc_nor_rtos_load(&rtos_header, rtos_offset))
		hang();

	flush_cache_all();

	rtos_raw_start(&rtos_header, &spl_rtos_args);

#ifdef CONFIG_JZ_SECURE_SUPPORT
	int ret = 0;
	ret = secure_scboot(spl_image.load_addr - 2048, spl_image.load_addr);
	if (ret) {
		printf("Error spl secure load kernel\n");
		hang();
	}
#endif

	return cmdargs;
}

#endif

#ifdef CONFIG_SPL_OS_BOOT
void spl_sfc_nor_os_load(void)
{
	struct norflash_partitions partition;
	unsigned int bootimg_addr = 0;

	sfc_read_data(CONFIG_SPIFLASH_PART_OFFSET + sizeof(struct spi_nor_info) + sizeof(int) * 2, sizeof(struct norflash_partitions), (unsigned char*)&partition);
	bootimg_addr = get_part_offset_by_name(partition, CONFIG_SPL_OS_NAME);
	if (bootimg_addr == -1){
		printf("kernel not found: "CONFIG_SPL_OS_NAME"\n");
		hang();
	}

	spl_load_kernel(bootimg_addr);
}
#endif

#ifdef CONFIG_SPL_ALIOS_BOOT
typedef struct alios_image_header {
	unsigned int header_size;/* Image Header Size	*/
	unsigned int image_crc;	/* Image CRC Checksum	*/
	unsigned int image_size;	/* Image Data Size		*/
} alios_image_header_t;

#define RTOSA 22
#define RTOSB 23

typedef struct alios_boot_info_param {
	uint32_t version;
	uint32_t state;
	uint32_t partition;
	uint32_t error_code;
	uint32_t update_size;
	uint32_t rtosa_start;
	uint32_t rtosa_size;
	uint32_t rtosb_start;
	uint32_t rtosb_size;
	uint32_t udisk_start;
	uint32_t udisk_size;
} alios_boot_info_param_t;

void spl_sfc_nor_alios_load(void)
{
	unsigned int aos_img_addr = 0;
	int crc1, crc2;

	unsigned int buffer[32] = {0};
	alios_image_header_t *header;
	unsigned int header_size = sizeof(buffer);

	unsigned int buffer1[48] = {0};
	alios_boot_info_param_t *param;
	unsigned int param_size = sizeof(buffer1);
	int crc_try = 3;

	sfc_read_data(CONFIG_ALIOS_BOOT_INFO_OFFSET, sizeof(buffer1), CONFIG_SYS_TEXT_BASE);
	memcpy(buffer1, (alios_boot_info_param_t *)(CONFIG_SYS_TEXT_BASE), sizeof(alios_boot_info_param_t));
	param = (alios_boot_info_param_t *)buffer1;

	debug("param:\n");
	debug("param->version:   %x\n", param->version);
	debug("param->state:     %x\n", param->state);
	debug("param->partition: %x\n", param->partition);
	debug("param->error_code:%x\n", param->error_code);
	debug("param->update_size:%x\n",param->update_size);
	debug("param->rtosa_start:%x\n",param->rtosa_start);
	debug("param->rtosa_size:%x\n", param->rtosa_size);
	debug("param->rtosb_start:%x\n",param->rtosb_start);
	debug("param->rtosb_size:%x\n", param->rtosb_size);
	debug("param->udisk_start:%x\n",param->udisk_start);
	debug("param->udisk_size:%x\n", param->udisk_size);

change_part:
	if(param->partition == RTOSA) {
		printf("boot rtos-A\n");
		aos_img_addr = param->rtosa_start;
	} else if (param->partition == RTOSB) {
		printf("boot rtos-B\n");
		aos_img_addr = param->rtosb_start;
	} else {
		printf("boot partition type error!\n");
		hang();
	}

	/* read alios image head */
	debug("aos_img_addr: 0x%x\n", aos_img_addr);
	sfc_read_data(aos_img_addr, sizeof(buffer), CONFIG_SYS_TEXT_BASE);
	memcpy(buffer, (struct alios_image_header *)(CONFIG_SYS_TEXT_BASE), sizeof(alios_image_header_t));
	header = (alios_image_header_t *)buffer;

	debug("header:\n");
	debug("header_size: %x\n",	 header->header_size);
	debug("image_crc: %x\n",	 header->image_crc);
	debug("image_size: %x\n",	 header->image_size);

	spl_image.size = header->image_size;
	spl_image.entry_point = CONFIG_SYS_TEXT_BASE;
	spl_image.load_addr = CONFIG_SYS_TEXT_BASE;
	spl_image.os = IH_OS_ALIOS;
	spl_image.name = "Alios";
	crc1 = header->image_crc;
	sfc_read_data(aos_img_addr + header->header_size, spl_image.size, spl_image.load_addr);
	crc2 = crc32(0, spl_image.load_addr, spl_image.size);
	if(crc1 != crc2){
		if(param->partition == RTOSA) {
			printf("crc error !!! goto rtos-B\n");
			param->partition = RTOSB;
		} else if(param->partition == RTOSB) {
			printf("crc error !!! goto rtos-A\n");
			param->partition = RTOSA;
		}
		if(crc_try--)
			goto change_part;
		
		printf("crc error, boot failed!\n");
		hang();
	}
	jump_to_image_no_args(&spl_image);
}
#endif

#ifdef CONFIG_OTA_VERSION20
void spl_ota_load_image(void)
{
	struct image_header *header;

	unsigned int bootimg_addr = 0;
	unsigned int bootimg_size = 0;
	struct norflash_partitions partition;
	int i;

	unsigned int nv_rw_addr;
	unsigned int nv_rw_size;
	unsigned int src_addr, updata_flag;
	unsigned nv_buf[2];
	int count = 8;

	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);
	//memset(header, 0, sizeof(struct image_header));

	sfc_read_data(CONFIG_SPIFLASH_PART_OFFSET + sizeof(struct spi_nor_info) + sizeof(int) * 2, sizeof(struct norflash_partitions), (unsigned char*)&partition);

	bootimg_addr = get_part_offset_by_name(partition, CONFIG_SPL_OS_NAME);
	if (bootimg_addr == -1){
		printf("kernel not found: "CONFIG_SPL_OS_NAME"\n");
		hang();
	}

	bootimg_size = get_part_size_by_name(partition, CONFIG_SPL_OS_NAME);
	if (bootimg_size == -1){
		printf("kernel not found: "CONFIG_SPL_OS_NAME"\n");
		hang();
	}

	nv_rw_addr = get_part_offset_by_name(partition, CONFIG_PAR_NV_NAME);
	if (nv_rw_addr == -1){
		printf("nv_rw not found: "CONFIG_PAR_NV_NAME"\n");
		hang();
	}

	nv_rw_size = get_part_size_by_name(partition, CONFIG_PAR_NV_NAME);
	if (nv_rw_size == -1){
		printf("nv_rw not found: "CONFIG_PAR_NV_NAME"\n");
		hang();
	}

	nv_map_area((unsigned int)&src_addr, nv_rw_addr, nv_rw_size);
	sfc_read_data(src_addr, count, (unsigned char *)nv_buf);
	updata_flag = nv_buf[1];
	if((updata_flag & 0x3) != 0x3)
	{
		spl_load_kernel(bootimg_addr);
	} else {
		header->ih_name[IH_NMLEN - 1] = 0;
		spl_parse_image_header(header);
		sfc_read_data(CONFIG_UBOOT_OFFSET, CONFIG_SYS_MONITOR_LEN,(unsigned char *)CONFIG_SYS_TEXT_BASE);
	}
}
#endif

#ifdef CONFIG_BOOT_VMLINUX
void spl_vmlinux_load(void)
{
	unsigned int bootimg_addr = 0;
	unsigned int bootimg_size = 0;
	struct norflash_partitions partition;

	sfc_read_data(CONFIG_SPIFLASH_PART_OFFSET + sizeof(struct spi_nor_info) + sizeof(int) * 2, sizeof(struct norflash_partitions), (unsigned char *)&partition);

	bootimg_addr = get_part_offset_by_name(partition, CONFIG_SPL_OS_NAME);
	if (bootimg_addr == -1) {
		printf("kernel not found: "CONFIG_SPL_OS_NAME"\n");
		hang();
	}

	bootimg_size = get_part_size_by_name(partition, CONFIG_SPL_OS_NAME);
	if (bootimg_size == -1) {
		printf("kernel not found: "CONFIG_SPL_OS_NAME"\n");
		hang();
	}

	spl_image.os = IH_OS_LINUX;
	spl_image.entry_point = CONFIG_LOAD_ADDR;
	sfc_read_data(bootimg_addr, bootimg_size, (unsigned char *)CONFIG_LOAD_ADDR);
}
#endif


#ifdef CONFIG_SPL_OS_OTA_BOOT
static char *spl_sfc_nor_os_ota_load(void)
{
	struct norflash_partitions partition;
	unsigned int img_addr = 0;
	int is_kernel2=0;
	const char *kernel_name=CONFIG_SPL_OS_NAME;

	sfc_read_data(CONFIG_SPIFLASH_PART_OFFSET + sizeof(struct spi_nor_info) + sizeof(int) * 2, sizeof(struct norflash_partitions), (unsigned char *)&partition);
	img_addr = get_part_offset_by_name(partition, CONFIG_SPL_OTA_NAME);
	if (img_addr != -1) {
		char buf[128];
		const char *kernel2 = "ota:"CONFIG_SPL_OS_NAME2;
		sfc_read_data(img_addr, sizeof(buf), (unsigned char *)buf);
		if (!strncmp(kernel2, buf, strlen(kernel2))) {
			is_kernel2 = 1;
			kernel_name=CONFIG_SPL_OS_NAME2;
		}
	}

	img_addr = get_part_offset_by_name(partition, kernel_name);
	if (img_addr == -1) {
		printf("kernel not found: "CONFIG_SPL_OS_NAME"\n");
		hang();
	}

	debug("kernel:%s %x\n", kernel_name, img_addr);

	spl_load_kernel(img_addr);

	if (is_kernel2)
		return CONFIG_SYS_SPL_ARGS_ADDR2;
	else
		return CONFIG_SYS_SPL_ARGS_ADDR;
}
#endif

#ifdef CONFIG_BOOT_RTOS_OTA
static void spl_sfc_nor_rtos_ota_boot(void)
{
	unsigned int ota_offset;
	unsigned int offset;
	struct norflash_partitions partition;
	sfc_read_data(CONFIG_SPIFLASH_PART_OFFSET + sizeof(struct spi_nor_info) + sizeof(int) * 2, sizeof(struct norflash_partitions), (unsigned char *)&partition);

	ota_offset = get_part_offset_by_name(partition, CONFIG_SPL_OTA_NAME);
	if (ota_offset != -1) {
		char buf[128];
		const char *kernel2 = "ota:"CONFIG_SPL_OS_NAME2;
		sfc_read_data(ota_offset, sizeof(buf), (unsigned char *)buf);
		if (strncmp(kernel2, buf, strlen(kernel2))) {
			return;
		}

		offset = get_part_offset_by_name(partition, CONFIG_SPL_RTOS_OTA_NAME);
		if (offset == -1) {
			printf("rtos not found: "CONFIG_SPL_RTOS_OTA_NAME"\n");
			return;
		}

		if (spl_sfc_nor_rtos_load(&rtos_header, offset))
			return;

        flush_cache_all();
		rtos_raw_start(&rtos_header, NULL);
	}
}
#endif

char* spl_sfc_nor_load_image(void)
{
	sfc_init();
	spl_rtos_args.os_boot_args = NULL;
	spl_rtos_args.card_params = NULL;
#ifdef CONFIG_BOOT_RTOS_OTA
	spl_sfc_nor_rtos_ota_boot();
#endif

#ifdef CONFIG_BOOT_VMLINUX
	spl_vmlinux_load();
	return NULL;
#elif defined(CONFIG_OTA_VERSION20)
	return spl_ota_load_image();
	return NULL;
#elif defined(CONFIG_SPL_RTOS_LOAD_KERNEL)
	return spl_sfc_nor_boot_rtos_load_os();
#elif defined(CONFIG_SPL_OS_OTA_BOOT)
	return spl_sfc_nor_os_ota_load();
#elif defined(CONFIG_SPL_OS_BOOT)
	spl_sfc_nor_os_load();
	return NULL;
#elif defined(CONFIG_SPL_RTOS_BOOT)
	spl_sfc_nor_rtos_boot();
	return NULL;
#elif defined(CONFIG_SPL_ALIOS_BOOT)
	spl_sfc_nor_alios_load();
	return NULL;
#else
	{
		struct image_header *header;
		header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

		header->ih_name[IH_NMLEN - 1] = 0;
		spl_parse_image_header(header);
		sfc_read_data(CONFIG_UBOOT_OFFSET, CONFIG_SYS_MONITOR_LEN,(unsigned char *)CONFIG_SYS_TEXT_BASE);
	}
	return NULL;
#endif
}

