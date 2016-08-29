#include <common.h>
#include <config.h>
#include <spl.h>
#include <spi.h>
#include <asm/io.h>
#include <nand.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spi.h>
#include <asm/arch/clk.h>

#include "../../drivers/spi/jz_spi.h"

static struct spi_quad_mode *quad_mode = NULL;
/* Borrow the uboot space */
static struct nor_sharing_params *pdata = CONFIG_SYS_TEXT_BASE;

unsigned int quad_mode_is_set = 0;

#ifdef CONFIG_SPI_QUAD
#define SFC_QUAD_MODE 1
#else
#define SFC_QUAD_MODE 0
#endif

struct jz_sfc {
	unsigned int  addr;
	unsigned int  len;
	unsigned int  cmd;
	unsigned int  addr_plus;
	unsigned char channel;
	unsigned char dir;	/* read or write */
	unsigned char mode;
	unsigned char daten;
	unsigned char addr_size;
	unsigned char pollen;
	unsigned char phase;
	unsigned char dummy_byte;
};

static inline uint32_t jz_sfc_readl(unsigned int offset)
{
	return readl(SFC_BASE + offset);
}

static inline void jz_sfc_writel(unsigned int value, unsigned int offset)
{
	writel(value, SFC_BASE + offset);
}
static inline void is_end_and_clear(void)
{
	unsigned int val;
	val = jz_sfc_readl(SFC_SR);
	while (!(val & END)){
		val = jz_sfc_readl(SFC_SR);
	}
	if ((jz_sfc_readl(SFC_SR)) & END)
		jz_sfc_writel(CLR_END,SFC_SCR);
}
static void read_fifo_data(unsigned int *data, unsigned int length)
{
	unsigned int n;
	while(!(jz_sfc_readl(SFC_SR) & RECE_REQ))
		;
	jz_sfc_writel(CLR_RREQ, SFC_SCR);
	for(n =0 ; n < length; n ++) {
		*data = jz_sfc_readl(SFC_DR);
		data++;
	}
}
static void write_fifo_data(unsigned int *data, unsigned int length)
{
	unsigned int n;
	while(!(jz_sfc_readl(SFC_SR) & TRAN_REQ))
		;
	jz_sfc_writel(CLR_TREQ, SFC_SCR);
	for(n =0 ; n < length; n ++) {
		jz_sfc_writel(*data, SFC_DR);
		data++;
	}
}
static int sfc_data_ops(unsigned int *data, unsigned int size, void (*ops)(unsigned int*, unsigned int))
{
	unsigned int *end = data + (size + 3) / 4;

	while(data < end - THRESHOLD) {
		ops(data, THRESHOLD);
		data += THRESHOLD;
	}
	if(data < end) {
		ops(data, end - data);
	}
	is_end_and_clear();
	return 0;
}

static inline void sfc_set_transfer(struct jz_sfc *hw)
{
	unsigned int val;

	val = jz_sfc_readl(SFC_GLB);
	val &= ~TRAN_DIR;
	val |= hw->dir << TRAN_DIR_OFFSET;
	jz_sfc_writel(val,SFC_GLB);

	jz_sfc_writel(hw->len, SFC_TRAN_LEN);

	val = jz_sfc_readl(SFC_TRAN_CONF(hw->channel));
	val &= ~(ADDR_WIDTH_MSK | CMDEN | DATEEN | CMD_MSK \
		 | TRAN_CONF_DMYBITS_MSK | TRAN_MODE_MSK);
	val |= (hw->addr_size << ADDR_WIDTH_OFFSET | CMDEN \
		| hw->daten << TRAN_DATEEN_OFFSET | hw->cmd << CMD_OFFSET \
		| hw->dummy_byte << TRAN_CONF_DMYBITS_OFFSET \
		| hw->mode << TRAN_MODE_OFFSET);
	jz_sfc_writel(val,SFC_TRAN_CONF(hw->channel));

	jz_sfc_writel(hw->addr, SFC_DEV_ADDR(hw->channel));
	jz_sfc_writel(hw->addr_plus,SFC_DEV_ADDR_PLUS(hw->channel));
}

static inline void sfc_send_cmd(struct jz_sfc *sfc)
{
	sfc_set_transfer(sfc);
	jz_sfc_writel(1 << 2,SFC_TRIG);
	jz_sfc_writel(START,SFC_TRIG);

	/*this must judge the end status*/
	if(sfc->daten == 0)
		is_end_and_clear();
}
static void sfc_set_quad_mode(void)
{
	unsigned char cmd[4];
	unsigned int buf = 0;
	unsigned int tmp = 0;
	int i = 10;
	struct jz_sfc sfc;

	if(quad_mode != NULL){
		cmd[0] = CMD_WREN;
		cmd[1] = quad_mode->WRSR_CMD;
		cmd[2] = quad_mode->RDSR_CMD;
		cmd[3] = CMD_RDSR;

		sfc.cmd = cmd[0];
		sfc.addr = 0;
		sfc.addr_size = 0;
		sfc.addr_plus = 0;
		sfc.dummy_byte = 0;
		sfc.daten = 0;
		sfc.len = 0;
		sfc.dir = GLB_TRAN_DIR_WRITE;
		sfc.channel = 0;
		sfc.mode = TRAN_SPI_STANDARD;
		sfc_send_cmd(&sfc);

		sfc.cmd = cmd[1];
		sfc.len = quad_mode->WD_DATE_SIZE;
		sfc.daten = 1;
		sfc_send_cmd(&sfc);
		sfc_data_ops(&quad_mode->WRSR_DATE,1, write_fifo_data);

		sfc.cmd = cmd[3];
		sfc.len = 1;
		sfc.daten = 1;
		sfc.dir = GLB_TRAN_DIR_READ;
		sfc_send_cmd(&sfc);
		sfc_data_ops(&tmp, 1, read_fifo_data);

		while(tmp & CMD_SR_WIP) {
			sfc.cmd = cmd[3];
			sfc.len = 1;
			sfc.daten = 1;
			sfc.dir = GLB_TRAN_DIR_READ;
			sfc_send_cmd(&sfc);
			sfc_data_ops(&tmp, 1, read_fifo_data);
		}
		sfc.cmd = cmd[2];
		sfc.len = quad_mode->WD_DATE_SIZE;
		sfc.daten = 1;
		sfc_send_cmd(&sfc);
		sfc_data_ops(&buf, 1, read_fifo_data);
		while(!(buf & quad_mode->RDSR_DATE)&&((i--) > 0)) {
			sfc.cmd = cmd[2];
			sfc.len = quad_mode->WD_DATE_SIZE;
			sfc.daten = 1;
			sfc_send_cmd(&sfc);
			sfc_data_ops(&buf, 1, read_fifo_data);
		}
		quad_mode_is_set = 1;
		/* printf("set quad mode is enable.the buf = %x\n",buf); */
	} else {
		printf("WARN!!!: the nor flash not support quad_mode\n");
	}
}

static void sfc_nor_read_params(unsigned int offset)
{
	unsigned int len;
	struct jz_sfc sfc;

	len = sizeof(struct nor_sharing_params);
	sfc.cmd = CMD_READ;
	sfc.addr = offset;
	sfc.addr_size = 3;
	sfc.addr_plus = 0;
	sfc.dummy_byte = 0;
	sfc.daten = 1;
	sfc.len = len;
	sfc.dir = GLB_TRAN_DIR_READ;
	sfc.channel = 0;
	sfc.mode = TRAN_SPI_STANDARD;

	sfc_send_cmd(&sfc);
	sfc_data_ops((unsigned int *)pdata, len, read_fifo_data);
}

static inline int get_nor_info(unsigned int offset)
{
#ifndef CONFIG_BURNER
	sfc_nor_read_params(offset);
	if(pdata->magic == NOR_MAGIC) {
		if(pdata->version == CONFIG_NOR_VERSION){
			quad_mode = &pdata->norflash_params.quad_mode;
			if(pdata->norflash_params.addrsize == 4)
				printf("WARNNING: xImage not support addr > 16M\n");
			return 0;
		}
	}
#endif
	return -1;
}

static void reset_nor()
{
	struct jz_sfc sfc;

	sfc.cmd = 0x66;
	sfc.addr = 0;
	sfc.addr_size = 0;
	sfc.addr_plus = 0;
	sfc.dummy_byte = 0;
	sfc.daten = 0;
	sfc.len = 0;
	sfc.dir = GLB_TRAN_DIR_READ;
	sfc.channel = 0;
	sfc.mode = TRAN_SPI_STANDARD;
	sfc_send_cmd(&sfc);
	sfc.cmd = 0x99;
	sfc_send_cmd(&sfc);
	udelay(60);
}

void sfc_init()
{
	int err;
	unsigned int val;

	/*the sfc clk is 1/2 ssi clk */
	clk_set_rate(SSI, 150000000);

	jz_sfc_writel(3 << 1 ,SFC_TRIG);

	val = jz_sfc_readl(SFC_GLB);
	val &= ~(TRAN_DIR | OP_MODE | THRESHOLD_MSK);
	val |= (WP_EN | THRESHOLD << THRESHOLD_OFFSET);
	jz_sfc_writel(val,SFC_GLB);

	val = jz_sfc_readl(SFC_DEV_CONF);
	val &= ~(CMD_TYPE | CPHA | CPOL | SMP_DELAY_MSK |
		 THOLD_MSK | TSETUP_MSK | TSH_MSK);
	val |= (CEDL | HOLDDL | WPDL | (1 << TSETUP_OFFSET) | (1 << SMP_DELAY_OFFSET));
	jz_sfc_writel(val,SFC_DEV_CONF);

	val = CLR_END | CLR_TREQ | CLR_RREQ | CLR_OVER | CLR_UNDER;
	jz_sfc_writel(val,SFC_INTC);
	jz_sfc_writel(0,SFC_CGE);

	reset_nor();

	err = get_nor_info(CONFIG_SPIFLASH_PART_OFFSET);
	if(err < 0){
		printf("the sfc quad mode err,check your soft code\n");
	} else {
		sfc_set_quad_mode();
	}
}
void sfc_read_data(unsigned int offset, unsigned int size, unsigned int data)
{
	unsigned char cmd;
	struct jz_sfc sfc;

	cmd = CMD_READ;
	sfc.addr = offset;
	sfc.addr_size = 3;//pdata.norflash_params.addrsize;
	sfc.addr_plus = 0;
	sfc.daten = 1;
	sfc.len = size;
	sfc.dir = GLB_TRAN_DIR_READ;
	sfc.channel = 0;
	sfc.mode = TRAN_SPI_STANDARD;
	sfc.dummy_byte = 0;

	if(SFC_QUAD_MODE == 1){
		cmd  = quad_mode->cmd_read;
		sfc.mode = quad_mode->sfc_mode;
		sfc.dummy_byte = quad_mode->dummy_byte;
	}

	sfc.cmd = cmd;
	sfc_send_cmd(&sfc);
	sfc_data_ops((unsigned int *)data, size, read_fifo_data);
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
	memset(header, 0, sizeof(struct image_header));
	sfc_init();
#ifdef CONFIG_SPL_OS_BOOT
	sfc_read_data(CONFIG_SPI_NORFLASH_PART_OFFSET, sizeof(struct norflash_partitions), (unsigned int*)&partition);
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
