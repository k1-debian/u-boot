#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/clk.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spinand.h>
#include "spl.h"
#include <generated/sfc_timing_val.h>
#include <generated/sfc_nand_params.h>
#include "spl_rtos.h"
#include "spl_rtos_argument.h"
#include "spl_riscv.h"

#ifdef CONFIG_OTA_VERSION30
#include "spl_ota_kunpeng.h"
#endif

#define SPINAND_PARAM_SIZE			1024

static struct spl_rtos_argument spl_rtos_args;
static struct rtos_boot_os_args os_boot_args;

static struct spl_nand_param *curr_device;

#ifdef CONFIG_X2580
/* nand 未经测试 */
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
/* nand 未经测试 */
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

static void sfc_do_erase_blk(unsigned int page_addr)
{
    unsigned int read_buf = 0;
    struct jz_sfc sfc;
    memset(&sfc, 0, sizeof(sfc));

    /* write enable */
    SFC_SEND_COMMAND(&sfc, SPINAND_CMD_WREN, 0, 0, 0, 0, 0, 0);
    clear_end();

    /* erase block */
    SFC_SEND_COMMAND(&sfc, SPINAND_CMD_ERASE_128K, 0, page_addr, 3, 0, 0, 0);
    clear_end();

    /* get feature */
    do {
        SFC_SEND_COMMAND(&sfc, SPINAND_CMD_GET_FEATURE, 1, SPINAND_ADDR_STATUS, 1, 0, 1, 0);
        sfc_read_data(&read_buf, 1);
    }while(read_buf & 0x1);
}

static void sfc_controler_init(void)
{
	unsigned int tmp;
#ifdef CONFIG_SFC_NAND_INIT_RATE
	clk_set_rate(SFC, CONFIG_SFC_NAND_INIT_RATE);
#else
	/* default: SFC rate 50MHz */
	clk_set_rate(SFC, 200000000L);
#endif

	tmp = sfc_readl(SFC_GLB);
	tmp &= ~(GLB_THRESHOLD_MSK);
	tmp |= (THRESHOLD << GLB_THRESHOLD_OFFSET);
	sfc_writel(tmp, SFC_GLB);

#ifdef CONFIG_X2580
	ingenic_sfc_gpio_slew_driver_strength();
#endif

	/* default: tSH--5cycle, tSETUP--1/2cycle, tHOLD--1/2cycle */
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

	/* plane select */
	if(curr_device->device_id == 0x22			/* MX35LF2GE4AB */
			|| curr_device->device_id == 0x72	/* DS35Q2GAXXX,	ZD35Q2GA */
		        || curr_device->device_id == 0xF1	/* DS35X1GBXXX*/
			|| curr_device->device_id == 0xF2	/* DS35Q2GBXXX */
			|| curr_device->device_id == 0x24)	/* XT26G02E */
		column |= (((page >> 6) & 1) << 12);

#ifdef CONFIG_SFC_QUAD
	#ifdef CONFIG_X2580
	x2580_sfc_change_io_function(1);
	#endif
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
		if (nand_param[i].device_id >0x0 && nand_param[i].device_id<=0xff &&
						nand_param[i].id_manufactory == id[0] &&
						nand_param[i].device_id == id[1]) {
			curr_device = &nand_param[i];
			break;
		}
		else if(nand_param[i].device_id >0xff && nand_param[i].device_id <= 0xffff &&
						nand_param[i].id_manufactory == id[0] &&
						nand_param[i].device_id == (id[2] | (id[1]<<8))) {
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
	unsigned char id[3] = {0};
	unsigned char i;
	for(i = 0; i < sizeof(addrlen); i++) {
		SFC_SEND_COMMAND(sfc, SPINAND_CMD_RDID, 3, 0, addrlen[i], 0, 1, 0);
		sfc_read_data((unsigned int *)id, 3);
		if (!probe_id_list(id)){
			    break;
		}
	}
	if(i == sizeof(addrlen)) {
		debug("ERR: don`t support this kind of nand device, \
			please add it\n");
		return -ENODEV;
	}
#ifndef CONFIG_DDR_DRVODT_DEBUG
	printf("%d, VID=0x%x, PID=0x%x\n", __LINE__, id[0], id[1]);
#endif
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
			debug("bad block %d\n", pageaddr / CONFIG_SPI_NAND_PPB);
			src_addr += CONFIG_SPI_NAND_PPB * pagesize;
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
			src_addr += CONFIG_SPI_NAND_PPB * pagesize;
			continue;
		}

		buf += rlen;
		src_addr += rlen;
		count -= rlen;
		retry_count = 5;
	}

	return 0;
}

struct jz_sfcnand_partition_param *get_partitions(void)
{
	struct jz_sfcnand_burner_param *burn_param;
	static struct jz_sfcnand_partition_param partitions;

	/*read param*/
	sfc_nand_load(CONFIG_SPIFLASH_PART_OFFSET, SPINAND_PARAM_SIZE, CONFIG_SYS_TEXT_BASE);
	burn_param = (void *)(CONFIG_SYS_TEXT_BASE);
	partitions.num_partition = burn_param->partition_num;
	partitions.partition = (struct jz_sfcnand_partition *)&burn_param->partition;

	return &partitions;
}

unsigned int get_part_offset_by_name(struct jz_sfcnand_partition_param *partitions, char *name)
{
	int i = 0;

	for(i = 0; i < partitions->num_partition; i++) {
		if (!strncmp(partitions->partition[i].name, name, strlen(name))) {
			return partitions->partition[i].offset;
		}
	}

	return -1;
}

struct jz_sfcnand_partition *get_part_by_name(struct jz_sfcnand_partition_param *partitions, char *name)
{
	int i = 0;

	for(i = 0; i < partitions->num_partition; i++) {
		if (!strncmp(partitions->partition[i].name, name, strlen(name))) {
			return &partitions->partition[i];
		}
	}

	return NULL;
}

#ifdef CONFIG_JZ_SECURE_SUPPORT
extern int secure_scboot (void *, void *);
extern int is_security_boot(void);

#ifdef CONFIG_JZ_SECURE_ROOTFS
#define LOAD_ROOTFS_ADDR 0x82000000
static void secure_check_hash_rootfs(struct jz_sfcnand_partition_param *partitions)
{
	unsigned int signature_offset;
	unsigned int rootfs_offset;
	unsigned int code_len;
	unsigned int *ptr = (unsigned int *)(LOAD_ROOTFS_ADDR - 2048);
	int ret;

	signature_offset = get_part_offset_by_name(partitions, CONFIG_SPL_SIG_NAME);
	rootfs_offset = get_part_offset_by_name(partitions, CONFIG_SPL_ROOTFS_NAME);

	if (signature_offset == -1 || rootfs_offset == -1){
		printf("sig or rootfs partitions not found\n");
		hang();
	}

	sfc_nand_load(signature_offset, 2048, LOAD_ROOTFS_ADDR - 2048);
	code_len = ptr[128];

	sfc_nand_load(rootfs_offset, code_len, LOAD_ROOTFS_ADDR);

	ret = secure_scboot(LOAD_ROOTFS_ADDR - 2048, LOAD_ROOTFS_ADDR);
	if(ret) {
		printf("Error check rootfs hash.\n");
		hang();
	}
}
#endif
#endif

void spl_load_kernel(long offset)
{

	struct image_header *header;

#ifdef CONFIG_JZ_SECURE_SUPPORT
	int ret;
	header = (struct image_header *)(CONFIG_SYS_SC_TEXT_BASE);

	sfc_nand_load(offset, sizeof(struct image_header) + sizeof(int), CONFIG_SYS_TEXT_BASE);
	header->ih_name[IH_NMLEN - 1] = 0;

	spl_parse_image_header(header);

	sfc_nand_load(offset, spl_image.size, spl_image.load_addr - 2048);
	ret = secure_scboot(spl_image.load_addr - 2048, spl_image.load_addr);
	if(ret) {
		printf("Error spl secure load kernel.\n");
		hang();
	}
#else
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

	sfc_nand_load(offset, sizeof(struct image_header), CONFIG_SYS_TEXT_BASE);
	header->ih_name[IH_NMLEN - 1] = 0;
	spl_parse_image_header(header);
	sfc_nand_load(offset, spl_image.size, spl_image.load_addr);
#endif

}

void sfc_erase_data(unsigned int addr, unsigned int len)
{
    unsigned int end;
    unsigned int page_addr;
    unsigned int blocksize = 0x20000;   /* CONFIG_SPI_NAND_BPP * CONFIG_SPI_NAND_PPB */


    if ((blocksize-1) & addr) {
        printf("erase error: address isn't aligned with blocks_size.\n");
        hang();
    }

    if ((blocksize-1) & len) {
        printf("erase error: len must be times of blocks_size.\n");
        hang();
	}


    end = addr + len;
    while (addr < end) {
        page_addr = addr / 2048;      /* CONFIG_SPI_NAND_BPP */
        sfc_do_erase_blk(page_addr);
        addr += blocksize;
    }
}

static volatile int sfc_is_inited = 0;

void sfc_init(void)
{
	if (sfc_is_inited)
		return;

	sfc_is_inited = 1;

	sfc_controler_init();
	spinand_init();
}

#ifdef CONFIG_SPL_OS_BOOT
void spl_sfc_nand_os_load(void)
{
	struct jz_sfcnand_partition_param *partitions;
	unsigned int bootimg_addr = 0;

	sfc_init();

	partitions = get_partitions();
	bootimg_addr = get_part_offset_by_name(partitions, CONFIG_SPL_OS_NAME);
	if (bootimg_addr == -1){
		printf("kernel not found: "CONFIG_SPL_OS_NAME"\n");
		hang();
	}

	/* read image head */
	spl_load_kernel(bootimg_addr);
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

static int spl_sfc_rtos_load(struct rtos_header *rtos, unsigned int offset)
{
	sfc_nand_load(offset, sizeof(*rtos), (unsigned int)rtos);
	if (rtos_check_header(rtos))
		return -1;

#ifdef CONFIG_JZ_SECURE_SUPPORT
	sfc_nand_load(offset, rtos->img_end - rtos->img_start, rtos->img_start);
	int ret = secure_scboot(rtos->img_start + sizeof(struct rtos_header), rtos->img_start);
	if(ret) {
		printf("Error rtos decryption.\n");
		hang();
	}
#else
	sfc_nand_load(offset, rtos->img_end - rtos->img_start, rtos->img_start);
#endif
	return 0;
}

#endif

#ifdef CONFIG_SPL_RTOS_BOOT
static void spl_sfc_rtos_boot(void)
{
	unsigned int rtos_offset = CONFIG_RTOS_OFFSET;

	sfc_init();

#ifdef CONFIG_SPL_OS_OTA_BOOT
	const char *rtos_name = CONFIG_SPL_RTOS_NAME;
	struct jz_sfcnand_partition_param *partitions = get_partitions();

	rtos_offset = get_part_offset_by_name(partitions, CONFIG_SPL_OTA_NAME);
	if (rtos_offset != -1) {
		char buf[128];
		const char *kernel2 = "ota:"CONFIG_SPL_OS_NAME2;
		sfc_nand_load(rtos_offset, sizeof(buf), (unsigned int)buf);
		if (!strncmp(kernel2, buf, strlen(kernel2))) {
			rtos_name = CONFIG_SPL_RTOS_NAME2;
		}
	}

	rtos_offset = get_part_offset_by_name(partitions, rtos_name);
	if (rtos_offset == -1) {
		printf("rtos not found: "CONFIG_SPL_RTOS_NAME"\n");
		hang();
	}

	debug("rtos:%s %x\n", rtos_name, rtos_offset);
#else
	#ifdef CONFIG_SPL_RTOS_NAME
	struct jz_sfcnand_partition_param *partitions = get_partitions();

	rtos_offset = get_part_offset_by_name(partitions, CONFIG_SPL_RTOS_NAME);
	if (rtos_offset == -1) {
		printf("rtos not found: "CONFIG_SPL_RTOS_NAME"\n");
		printf("use rtos default offset_addr:%d\n", CONFIG_RTOS_OFFSET);
		rtos_offset = CONFIG_RTOS_OFFSET;
	}
	#else
		rtos_offset = CONFIG_RTOS_OFFSET;
	#endif
#endif

	if (spl_sfc_rtos_load(&rtos_header, rtos_offset))
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

static void spl_sfc_nand_cfg_os_args(struct jz_sfcnand_partition_param *partitions, char *kernel_name, char *cmdargs)
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
	sfc_nand_load(img_addr, sizeof(struct image_header) + sizeof(int), CONFIG_SYS_TEXT_BASE);
#else
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);
	sfc_nand_load(img_addr, sizeof(struct image_header), CONFIG_SYS_TEXT_BASE);
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
static char *spl_sfc_nand_boot_rtos_load_os(void)
{
	struct jz_sfcnand_partition_param *partitions;
	const char *kernel_name = CONFIG_SPL_OS_NAME;
	const char *rtos_name = CONFIG_SPL_RTOS_NAME;
	char *cmdargs = CONFIG_SYS_SPL_ARGS_ADDR;

	sfc_init();

	partitions = get_partitions();

#ifdef CONFIG_SPL_OS_OTA_BOOT
	unsigned int ota_addr = 0;
	ota_addr = get_part_offset_by_name(partitions, CONFIG_SPL_OTA_NAME);
	if (ota_addr != -1) {
		char buf[128];
		const char *kernel2 = "ota:"CONFIG_SPL_OS_NAME2;
		sfc_nand_load(ota_addr, sizeof(buf), (unsigned int)buf);
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
	spl_sfc_nand_cfg_os_args(partitions, kernel_name, cmdargs);

	unsigned int rtos_offset = 0;
	rtos_offset = get_part_offset_by_name(partitions, rtos_name);
	if (rtos_offset == -1) {
		printf("rtos not found: "CONFIG_SPL_RTOS_NAME"\n");
		printf("use rtos default offset_addr:%d\n", CONFIG_RTOS_OFFSET);
		rtos_offset = CONFIG_RTOS_OFFSET;
	}
	debug("rtos:%s %x\n", rtos_name, rtos_offset);

	if (spl_sfc_rtos_load(&rtos_header, rtos_offset))
		hang();

	flush_cache_all();

	rtos_raw_start(&rtos_header, &spl_rtos_args);

#ifdef CONFIG_JZ_SECURE_SUPPORT
	int ret = 0;
	ret = secure_scboot(spl_image.load_addr - 2048, spl_image.load_addr);
	if (ret) {
		printf("Error spl secure load kernel.\n");
		hang();
	}
#endif
	return cmdargs;
}

#endif

#ifdef CONFIG_BOOT_RTOS
void spl_sfc_nand_rtos_load(void)
{
	struct jz_sfcnand_partition_param *partitions;
	struct jz_sfcnand_partition *partition;

	sfc_init();

	partitions = get_partitions();
	partition = get_part_by_name(partitions, CONFIG_SPL_OS_NAME);
	if (partition == NULL) {
		printf("kernel not found: "CONFIG_SPL_OS_NAME"\n");
		hang();
	}

	spl_image.entry_point = CONFIG_LOAD_ADDR;
	sfc_nand_load(partition->offset, partition->size, (unsigned int*)CONFIG_LOAD_ADDR);
}
#endif

#ifdef CONFIG_SPL_OS_OTA_BOOT
static char *spl_sfc_nand_os_ota_load(void)
{
	struct jz_sfcnand_partition_param *partitions;
	unsigned int img_addr = 0;
	int is_kernel2=0;
	const char *kernel_name=CONFIG_SPL_OS_NAME;

	sfc_init();

	partitions = get_partitions();
	img_addr = get_part_offset_by_name(partitions, CONFIG_SPL_OTA_NAME);
	if (img_addr != -1) {
		char buf[128];
		const char *kernel2 = "ota:"CONFIG_SPL_OS_NAME2;
		sfc_nand_load(img_addr, sizeof(buf), (unsigned int)buf);
		if (!strncmp(kernel2, buf, strlen(kernel2))) {
			is_kernel2 = 1;
			kernel_name=CONFIG_SPL_OS_NAME2;
		}
	}

	img_addr = get_part_offset_by_name(partitions, kernel_name);
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

#ifdef CONFIG_SPL_MCU_RTOS_BOOT
#ifdef CONFIG_X2600
#include "x2600_riscv.h"
#endif

void spl_nand_mcu_rtos_boot(void)
{
    unsigned int riscv_offset;

    struct jz_sfcnand_partition_param *partitions = get_partitions();

    struct jz_sfcnand_partition *riscv_part = get_part_by_name(partitions, "riscv");
    if (!riscv_part) {
        printf("not found riscv\n");
        return;
    }

    spl_load_riscv(sfc_nand_load, riscv_part->offset);

    flush_cache_all();

    riscv_reset();
}

#endif

#ifdef CONFIG_BOOT_RTOS_OTA
static void spl_sfc_nand_rtos_ota_boot(void)
{
	unsigned int ota_offset;
	unsigned int offset;
	struct jz_sfcnand_partition_param *partitions = get_partitions();

	ota_offset = get_part_offset_by_name(partitions, CONFIG_SPL_OTA_NAME);
	if (ota_offset != -1) {
		char buf[128];
		const char *kernel2 = "ota:"CONFIG_SPL_OS_NAME2;
		sfc_nand_load(ota_offset, sizeof(buf), (unsigned char *)buf);
		if (strncmp(kernel2, buf, strlen(kernel2))) {
			return;
		}

		offset = get_part_offset_by_name(partitions, CONFIG_SPL_RTOS_OTA_NAME);
		if (offset == -1) {
			printf("rtos not found: "CONFIG_SPL_RTOS_OTA_NAME"\n");
			return;
		}

		if (spl_sfc_rtos_load(&rtos_header, offset))
			return;

        flush_cache_all();
		rtos_raw_start(&rtos_header, NULL);
	}
}
#endif

#ifdef CONFIG_OTA_VERSION30
static struct ota_ops ota_ops = {
	.flash_init = sfc_init,
	.flash_read = sfc_nand_load,
	.flash_get_partitions = get_partitions,
	.flash_get_part_offset_by_name = get_part_offset_by_name,
	.flash_load_kernel = spl_load_kernel,
};
#endif

char* spl_sfc_nand_load_image(void)
{
	spl_rtos_args.os_boot_args = NULL;
	spl_rtos_args.card_params = NULL;

#ifdef CONFIG_BOOT_RTOS_OTA
	spl_sfc_nand_rtos_ota_boot();
#endif

#ifdef CONFIG_SPL_RTOS_LOAD_KERNEL
	return spl_sfc_nand_boot_rtos_load_os();
#endif

#if CONFIG_SPL_RTOS_BOOT
	spl_sfc_rtos_boot();
#endif

#ifdef CONFIG_OTA_VERSION30
	register_ota_ops(&ota_ops);
	return spl_ota_load_image();
#elif defined(CONFIG_BOOT_RTOS)
	spl_sfc_nand_rtos_load();
	return NULL;
#elif defined(CONFIG_SPL_OS_OTA_BOOT)
	return spl_sfc_nand_os_ota_load();
#elif defined(CONFIG_SPL_OS_BOOT)
	spl_sfc_nand_os_load();
#ifdef CONFIG_SPL_RISCV
	spl_nand_load_riscv();
	spl_start_riscv();
#endif

#ifdef CONFIG_SPL_MCU_RTOS_BOOT
    spl_nand_mcu_rtos_boot();
#endif
	return NULL;
#elif defined(CONFIG_SPL_RTOS_BOOT)
	hang();
	return NULL;
#else
	{
		struct image_header *header;
		header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);
		sfc_init();

		sfc_nand_load(CONFIG_UBOOT_OFFSET, CONFIG_SYS_MONITOR_LEN, (unsigned int)CONFIG_SYS_TEXT_BASE);
		spl_parse_image_header(header);
	}
	return NULL;
#endif
}
