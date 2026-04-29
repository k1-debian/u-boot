#include <common.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <asm/arch/cpm.h>

#ifndef __stage2_optimize
#define __stage2_optimize
#endif

#ifndef DEBUG
#ifdef SERIAL_DEBUG
#define DEBUG(s) serial_debug("%s", (s))
#define DEBUG_HEX(a) serial_debug("%x", (unsigned int)(a))
#else
#define DEBUG(s) do { } while (0)
#define DEBUG_HEX(a) do { } while (0)
#endif
#endif

#define xudelay(usec) udelay(usec)
#define xmdelay(msec) mdelay(msec)

#ifndef JZ_EXCLK
#define JZ_EXCLK 24000000
#endif

#define EXCLKDS_POC_PD		(1U << 31)
#define MSCCDR_DIV_MASK		0xff
#define MSCCDR_MPCS_MASK	(0x3U << 30)
#define MSCCDR_MPCS_EXCLK	(0x2U << 30)
#define MSCCDR_BUSY		(1U << 28)
#define MSCCDR_CE		(1U << 29)
#define MSCCDR_EXCK_E		(1U << 21)

#define MSC_BASE		MSC0_BASE
#define MSC_OFF			(MSC1_BASE - MSC0_BASE)

#define MSC_BLOCKSIZE_R(n)		((n) * MSC_OFF + 0x4)
#define MSC_BLOCKCOUNT_R(n)		((n) * MSC_OFF + 0x6)
#define MSC_ARGUMENT_R(n)		((n) * MSC_OFF + 0x8)
#define MSC_XFER_MODE_R(n)		((n) * MSC_OFF + 0xc)
#define MSC_CMD_R(n)			((n) * MSC_OFF + 0xe)
#define MSC_RESP01_R(n)			((n) * MSC_OFF + 0x10)
#define MSC_BUF_DATA_R(n)		((n) * MSC_OFF + 0x20)
#define MSC_PSTATE_REG(n)		((n) * MSC_OFF + 0x24)
#define MSC_HOST_CTRL1_R(n)		((n) * MSC_OFF + 0x28)
#define MSC_BGAP_CTRL_R(n)		((n) * MSC_OFF + 0x2a)
#define MSC_CLK_CTRL_R(n)		((n) * MSC_OFF + 0x2c)
#define MSC_TOUT_CTRL_R(n)		((n) * MSC_OFF + 0x2e)
#define MSC_SW_RST_R(n)			((n) * MSC_OFF + 0x2f)
#define MSC_NORMAL_INT_STAT_R(n)	((n) * MSC_OFF + 0x30)
#define MSC_ERROR_INT_STAT_R(n)		((n) * MSC_OFF + 0x32)
#define MSC_NORMAL_INT_STAT_EN_R(n)	((n) * MSC_OFF + 0x34)
#define MSC_ERROR_INT_STAT_EN_R(n)	((n) * MSC_OFF + 0x36)
#define MSC_MULTI_BLK_SEL			(1 << 5)
#define MSC_DATA_XFER_DIR_RD			(1 << 4)
#define MSC_AUTO_CMD12_ENABLE			(1 << 2)
#define MSC_BLOCK_COUNT_ENABLE			(1 << 1)

#define MSC_RESP_TYPE_SELECT_RESP_NO_RESP	(0 << 0)
#define MSC_RESP_TYPE_SELECT_RESP_LEN_136	(1 << 0)
#define MSC_RESP_TYPE_SELECT_RESP_LEN_48	(2 << 0)
#define MSC_RESP_TYPE_SELECT_RESP_LEN_48B	(3 << 0)

#define MSC_CMD_INHIBIT				(1 << 0)
#define MSC_DATA_INHIBIT			(1 << 1)
#define MSC_INTERNAL_CLK_EN			(1 << 0)
#define MSC_SD_CLK_EN				(1 << 2)
#define MSC_SW_RST_ALL				(1 << 0)
#define MSC_SW_RST_CMD				(1 << 1)
#define MSC_SW_RST_DAT				(1 << 2)
#define MSC_BUF_RD_READY_STAT			(1 << 5)
#define MSC_XFER_COMPLETE_STAT			(1 << 1)
#define MSC_CMD_COMPLETE_STAT			(1 << 0)
#define MSC_DATA_END_BIT_ERR_STAT		(1 << 6)
#define MSC_DATA_CRC_ERR_STAT			(1 << 5)
#define MSC_DATA_TOUT_ERR_STAT			(1 << 4)
#define MSC_CMD_IDX_ERR_STAT			(1 << 3)
#define MSC_CMD_END_BIT_ERR_STAT		(1 << 2)
#define MSC_CMD_CRC_ERR_STAT			(1 << 1)
#define MSC_CMD_TOUT_ERR_STAT			(1 << 0)

#define MSC_CMDAT_RESPONSE_NONE			0x0
#define MSC_CMDAT_RESPONSE_R1			0x1
#define MSC_CMDAT_RESPONSE_R1b			0x2
#define MSC_CMDAT_RESPONSE_R2			0x3
#define MSC_CMDAT_RESPONSE_R3			0x4

#define MSC_EXT_DAT_XFER_BIT			(1 << 5)
#define MSC_DAT_XFER_WIDTH_BIT			(1 << 1)

#define msc_readb(o)	readb(MSC_BASE + (o))
#define msc_readw(o)	readw(MSC_BASE + (o))
#define msc_readl(o)	readl(MSC_BASE + (o))
#define msc_writeb(v, o)	writeb((v), MSC_BASE + (o))
#define msc_writew(v, o)	writew((v), MSC_BASE + (o))
#define msc_writel(v, o)	writel((v), MSC_BASE + (o))

static inline void __cpm_start_msc(void)
{
	u32 tmp = cpm_readl(CPM_CLKGR0);

	tmp &= ~(CPM_CLKGR_MSC0 | CPM_CLKGR_MSC1);
	cpm_writel(tmp, CPM_CLKGR0);
}

static inline void __cpm_msc0_vol_3_3V(void)
{
	u32 tmp = cpm_readl(CPM_EXCLK_DS);

	tmp &= ~EXCLKDS_POC_PD;
	cpm_writel(tmp, CPM_EXCLK_DS);
}

static inline void __gpio_as_msc0_4bit(void)
{
	gpio_set_func(GPIO_PORT_D, GPIO_FUNC_0, 0x3f << 0);
}

static inline void __gpio_as_msc1_4bit_pd(void)
{
	gpio_set_func(GPIO_PORT_D, GPIO_FUNC_1, 0x3f << 6);
}

static inline void __gpio_as_msc1_4bit_pc(void)
{
	gpio_set_func(GPIO_PORT_C, GPIO_FUNC_0, 0x3f << 25);
}

static int rca;
static int ctl_num;
static int highcap;

//#define MSC_DEBUG

#ifdef MSC_DEBUG
static void dump_error_status(void)
{
	DEBUG("PSTATE_REG:");
	DEBUG_HEX(msc_readl(MSC_PSTATE_REG(ctl_num)));
	DEBUG("XFER_MODE_R:");
	DEBUG_HEX(msc_readw(MSC_XFER_MODE_R(ctl_num)));
	DEBUG("NORMAL_INT_STAT_EN_R:");
	DEBUG_HEX(msc_readw(MSC_NORMAL_INT_STAT_EN_R(ctl_num)));
	DEBUG("NORMAL_INT_STAT_R:");
	DEBUG_HEX(msc_readw(MSC_NORMAL_INT_STAT_R(ctl_num)));
	DEBUG("ERROR_INT_STAT_EN_R:");
	DEBUG_HEX(msc_readw(MSC_ERROR_INT_STAT_EN_R(ctl_num)));
	DEBUG("ERROR_INT_STAT_R:");
	DEBUG_HEX(msc_readw(MSC_ERROR_INT_STAT_R(ctl_num)));
	DEBUG("HOST_CTRL1_R:");
	DEBUG_HEX(msc_readb(MSC_HOST_CTRL1_R(ctl_num)));
}
#endif

static int cmd_err_stat_check(void)
{
	if(msc_readw(MSC_ERROR_INT_STAT_R(ctl_num))
			& (MSC_CMD_TOUT_ERR_STAT
				| MSC_CMD_CRC_ERR_STAT
				| MSC_CMD_END_BIT_ERR_STAT
				| MSC_CMD_IDX_ERR_STAT)){
#ifdef MSC_DEBUG
		dump_error_status();
#endif
		msc_writew(0xffff, MSC_ERROR_INT_STAT_R(ctl_num));
		return -1;
	}
	return 0;
}

static int xfer_err_stat_check(void)
{
	if(msc_readw(MSC_ERROR_INT_STAT_R(ctl_num))
			& (MSC_DATA_TOUT_ERR_STAT
				| MSC_DATA_CRC_ERR_STAT
				| MSC_DATA_END_BIT_ERR_STAT)){
#ifdef MSC_DEBUG
		dump_error_status();
#endif
		msc_writew(0xffff, MSC_ERROR_INT_STAT_R(ctl_num));
		return -1;
	}
	return 0;
}

static int wait_cmd_complete(void)
{
	u32 timeout = 1000;

	while(!(msc_readw(MSC_NORMAL_INT_STAT_R(ctl_num)) & MSC_CMD_COMPLETE_STAT) && --timeout) {
		xudelay(10);
	}

	msc_writew(MSC_CMD_COMPLETE_STAT, MSC_NORMAL_INT_STAT_R(ctl_num));

	if(!timeout) {
		DEBUG("cmd timeout err ...\n");
		return -1;
	}

	return 0;
}

static int wait_xfer_complete(void)
{
	u32 timeout = 1000;

	while(!(msc_readw(MSC_NORMAL_INT_STAT_R(ctl_num)) & MSC_XFER_COMPLETE_STAT) && --timeout) {
		xudelay(10);
	}

	msc_writew(MSC_XFER_COMPLETE_STAT, MSC_NORMAL_INT_STAT_R(ctl_num));

	if(!timeout) {
		DEBUG("xfer timeout err ...\n");
		return -1;
	}

	return 0;
}

static int wait_buf_rd(void)
{
	u32 timeout = 1000;

	while(!(msc_readw(MSC_NORMAL_INT_STAT_R(ctl_num)) & MSC_BUF_RD_READY_STAT) && --timeout) {
		xudelay(10);
	}

	msc_writew(MSC_BUF_RD_READY_STAT, MSC_NORMAL_INT_STAT_R(ctl_num));

	if(!timeout) {
		DEBUG("wait buf read state timeout err ...\n");
		return -1;
	}

	return 0;
}

static void msc_reset(int modules)
{
	u32 timeout = 1000;

	msc_writeb(modules, MSC_SW_RST_R(ctl_num));
	while((msc_readb(MSC_SW_RST_R(ctl_num)) & modules) && --timeout) {
		xudelay(10);
	}

	if(!timeout)
		DEBUG("sw reset timeout err ....\n");
}

static void msc_card_clks_en(void)
{
	u32 timeout = 1000;

	msc_writew(MSC_INTERNAL_CLK_EN, MSC_CLK_CTRL_R(ctl_num));
	while(!(msc_readw(MSC_CLK_CTRL_R(ctl_num)) & 0x2) && --timeout) {
		xudelay(10);
	}

	if(!timeout)
		DEBUG("clk wait stable timeout ....\n");

	msc_writew(MSC_SD_CLK_EN | MSC_INTERNAL_CLK_EN, MSC_CLK_CTRL_R(ctl_num));
}

static void msc_int_status_en(void)
{
	msc_writew(0xffff, MSC_NORMAL_INT_STAT_EN_R(ctl_num));
	msc_writew(0xffff, MSC_ERROR_INT_STAT_EN_R(ctl_num));
}

static void msc_timeout_set(void)
{
	/* timeout counter = tmclk * 2^(13 + value); value < 0xf */
	msc_writeb(0xe, MSC_TOUT_CTRL_R(ctl_num));
}

void msc0_pd0_3v3_init(void) {
	__gpio_as_msc0_4bit();
	__cpm_msc0_vol_3_3V();
}

void msc1_pd6_3v3_init(void)
{
	__gpio_as_msc1_4bit_pd();
}

void msc1_pc25_3v3_init(void)
{
	__gpio_as_msc1_4bit_pc();
}

/**
 *	mmc_cmd - host send cmd to device
 *	@arg: command argument
 *	@cmdidx: command index
 *	@cmdtype: command type (00/01/10/11
 *		normal cmd / suspend cmd / resume cmd / abort command)
 *	@cmdat: data present select, if set indicates that
 *		data is present and shall be transferred useing DAT line.
 *	@subcmd: distinguishes a main command or a sub command
 *	@rtype: response type
 */
static u8 __stage2_optimize *mmc_cmd(u32 arg, u32 cmdidx, u32 cmdtype, u32 cmdat, u32 subcmd, u16 rtype)
{
	static u32 buf[2] = {0};
	u16 cmd = 0;
	u32 mask = 0;
	u32 tmp = 0, i = 0;
	u8 *resp = (u8 *)buf;
	u32 timeout = 1000;

	buf[0] = 0;
	buf[1] = 0;

#ifdef MSC_DEBUG
	DEBUG(">>>>>>>> cmd id: ");
	DEBUG_HEX(cmdidx);
#endif
	/* chear all int status */
	msc_writew(0xffff, MSC_NORMAL_INT_STAT_R(ctl_num));
	msc_writew(0xffff, MSC_ERROR_INT_STAT_R(ctl_num));

	/* check cmd & data line */
	mask = MSC_CMD_INHIBIT | MSC_DATA_INHIBIT;

	/* We shouldn't wait for data inihibit for stop commands, even
	though they might use busy signaling */
	if (cmdidx == 12)
		mask &= ~MSC_DATA_INHIBIT;

	while ((msc_readl(MSC_PSTATE_REG(ctl_num)) & mask) && --timeout) {
		xudelay(10);
	}

	if (timeout == 0) {
		DEBUG("Controller never released inhibit bit(s).\n");
		goto exit;
	}

	/* set arg & cmd */
	cmd |= (cmdidx & 0x3f) << 8;
	cmd |= (cmdtype & 0x3) << 6;
	cmd |= (cmdat & 0x1) << 5;
	cmd |= (subcmd & 0x1) << 2;

	switch (rtype) {
		case MSC_CMDAT_RESPONSE_NONE:
			cmd |= MSC_RESP_TYPE_SELECT_RESP_NO_RESP;
			break;
		case MSC_CMDAT_RESPONSE_R1:
		case MSC_CMDAT_RESPONSE_R3:
			cmd |= MSC_RESP_TYPE_SELECT_RESP_LEN_48;
			break;
		case MSC_CMDAT_RESPONSE_R1b:
			cmd |= MSC_RESP_TYPE_SELECT_RESP_LEN_48B;
			break;
		case MSC_CMDAT_RESPONSE_R2:
			cmd |= MSC_RESP_TYPE_SELECT_RESP_LEN_136;
			break;
		default:
			cmd |= MSC_RESP_TYPE_SELECT_RESP_LEN_48;
			break;
	}

	msc_writel(arg, MSC_ARGUMENT_R(ctl_num));
	msc_writew(cmd, MSC_CMD_R(ctl_num));

	if(wait_cmd_complete()) {
		DEBUG("cmd send can't wait complete ...\n");
		goto exit;
	}

	if(cmd_err_stat_check()) {
		DEBUG("command state check error ...\n");
		goto exit;
	}

	/* get response */
	tmp = msc_readl(MSC_RESP01_R(ctl_num));
	for (i = 0; i < 4; i++) {
		resp[i + 1] = (tmp >> (i * 8)) & 0xff;
	}

exit:

	/* reset cmd & data line */
	if(!cmdat)
		msc_reset(MSC_SW_RST_DAT | MSC_SW_RST_CMD);

	return resp;
}

#ifndef FPGA_TEST
static void msc_clk_div_set(u32 div, u32 n)
{
	u32 tmp;
	u32 timeout = 200;
	u32 off;

	if(n==0)
		off = CPM_MSC0CDR;
	else
		off = CPM_MSC1CDR;

	/* Enable msc extclk */
	tmp = cpm_readl(CPM_MSC0CDR) | MSCCDR_EXCK_E;
	cpm_writel(tmp, CPM_MSC0CDR);

	tmp = cpm_readl(off);
	tmp &= ~(MSCCDR_DIV_MASK | MSCCDR_MPCS_MASK);
	tmp |= (MSCCDR_DIV_MASK & (div)) | MSCCDR_CE | MSCCDR_MPCS_EXCLK;

	cpm_writel(tmp, off);
	while((cpm_readl(off) & MSCCDR_BUSY) && --timeout) {
		xudelay(10);
	}
	if(!timeout)
		DEBUG("MSCCDR can't wait busy \n");

	tmp &= ~MSCCDR_CE;
	cpm_writel(tmp, off);
}
#endif

static void msc_set_xfer_bus_width(void)
{
	u32 val;

	/* bus width */
	val = msc_readb(MSC_HOST_CTRL1_R(ctl_num));

	val &= ~(MSC_EXT_DAT_XFER_BIT | MSC_DAT_XFER_WIDTH_BIT);
	val |= MSC_DAT_XFER_WIDTH_BIT;

	msc_writeb(val, MSC_HOST_CTRL1_R(ctl_num));
}

static int __stage2_optimize mmc_block_read(u32 start, u32 blkcnt, u32 *dst)
{
	u16 tmp = 0;
	u32 ret = 0;
	u32 cnt = 0;
	u32 nob = blkcnt;

	/* set block size and block count */
	msc_writew(512, MSC_BLOCKSIZE_R(ctl_num));
	msc_writew(nob, MSC_BLOCKCOUNT_R(ctl_num));
	msc_set_xfer_bus_width();

	/* CMD16 Sets the block length (in bytes) */
	mmc_cmd(512, 16, 0, 0, 0, MSC_CMDAT_RESPONSE_R1);

	/* set multiple block read and auto send CMD12 */
	tmp = MSC_MULTI_BLK_SEL | MSC_BLOCK_COUNT_ENABLE | MSC_DATA_XFER_DIR_RD | MSC_AUTO_CMD12_ENABLE;
	msc_writew(tmp, MSC_XFER_MODE_R(ctl_num));

	/**
	 * CMD18 multiple block read
	 * arg :
	 * SDSC : byte access
	 * SDHC/SDXC : sector access
	 */
	mmc_cmd((highcap ? start : (start * 512)), 18, 0, 1, 0, MSC_CMDAT_RESPONSE_R1);

	for (; nob > 0; nob--) {
		cnt = 128;

		if(wait_buf_rd()) {
			DEBUG("data xfer can't wait buf read ready ...\n");
			ret = -1;
			goto exit;
		}

		while(cnt--) {
			*dst++ = msc_readl(MSC_BUF_DATA_R(ctl_num));
		}
	}

	if(wait_xfer_complete()) {
		DEBUG("data xfer can't wait complete ...\n");
		ret = -1;
		goto exit;
	}

	if(xfer_err_stat_check()) {
		DEBUG("xfer state check error ...\n");
		ret = -1;
		goto exit;
	}

exit:
	msc_reset(MSC_SW_RST_DAT | MSC_SW_RST_CMD);
	return ret;
}

static void x2600_bootroom_mmc_gpio_init(void)
{
#if defined(CONFIG_JZ_MMC_MSC0) && defined(CONFIG_JZ_MMC_MSC0_PD)
	msc0_pd0_3v3_init();
#elif defined(CONFIG_JZ_MMC_MSC1) && defined(CONFIG_JZ_MMC_MSC1_PC)
	msc1_pc25_3v3_init();
#elif defined(CONFIG_JZ_MMC_MSC1) && defined(CONFIG_JZ_MMC_MSC1_PD)
	msc1_pd6_3v3_init();
#else
#error "unsupported bootroom helper MMC pinmux"
#endif
}

static void x2600_bootroom_mmc_select_ctl(void)
{
#ifdef CONFIG_JZ_MMC_MSC0
	ctl_num = 0;
#elif defined(CONFIG_JZ_MMC_MSC1)
	ctl_num = 1;
#else
#error "unsupported bootroom helper MMC controller"
#endif
}

static int x2600_bootroom_emmc_found(void)
{
	u8 *resp;
	u32 timeout = 100;
	u32 buswidth_arg;
	int status;

	highcap = 0;

	resp = mmc_cmd(0, 0, 0, 0, 0, MSC_CMDAT_RESPONSE_NONE);
	resp = mmc_cmd(0x40ff8000, 1, 0, 0, 0, MSC_CMDAT_RESPONSE_R3);

	while (--timeout && !(resp[4] & 0x80)) {
		xmdelay(10);
		resp = mmc_cmd(0x40ff8000, 1, 0, 0, 0, MSC_CMDAT_RESPONSE_R3);
	}

	if (!timeout) {
		DEBUG("emmc card init err ...\n");
		return -1;
	}

	if (resp[4] & 0x40) {
		DEBUG("the emmc card is a highcap card\n");
		highcap = 1;
	}

	resp = mmc_cmd(0, 2, 0, 0, 0, MSC_CMDAT_RESPONSE_R2);

	rca = 0x10 << 16;
	resp = mmc_cmd(rca, 3, 0, 0, 0, MSC_CMDAT_RESPONSE_R1);
	resp = mmc_cmd(rca, 7, 0, 0, 0, MSC_CMDAT_RESPONSE_R1);

	buswidth_arg = 0x3 << 24 | 183 << 16 | (0x1 << 8) | 0x1;
	resp = mmc_cmd(buswidth_arg, 6, 0, 0, 0, MSC_CMDAT_RESPONSE_R1b);

	timeout = 1000;
	do {
		resp = mmc_cmd(rca, 13, 0, 0, 0, MSC_CMDAT_RESPONSE_R1);
		status = resp[1] | (resp[2] << 8) |
			(resp[3] << 16) | (resp[4] << 24);
		if ((status & (0xf << 9)) != (7 << 9))
			break;
		xudelay(100);
	} while (--timeout);

	if (!timeout) {
		DEBUG("emmc switch bus width timeout ...\n");
		return -1;
	}

	msc_clk_div_set(0, ctl_num);

	return 0;
}

int x2600_bootroom_mmc_init(void)
{
	u32 clkrt = 0;

	x2600_bootroom_mmc_select_ctl();

	__cpm_start_msc();
	x2600_bootroom_mmc_gpio_init();

	msc_reset(MSC_SW_RST_ALL);
	msc_int_status_en();
	msc_timeout_set();
	msc_card_clks_en();

	clkrt = JZ_EXCLK / 200000 / 4 - 1;
	msc_clk_div_set(clkrt, ctl_num);

	if (x2600_bootroom_emmc_found()) {
		DEBUG("card init err ...\n");
		return -1;
	}

	return 0;
}

unsigned int x2600_bootroom_mmc_block_read(unsigned int start,
					   unsigned int blkcnt,
					   unsigned int *dst)
{
	return mmc_block_read(start, blkcnt, dst) ? 0 : blkcnt;
}
