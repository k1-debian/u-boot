/*
 * TPM_TIS_SPI TPM2-over-SPI helpers for measured boot gating.
 *
 * The implementation follows Linux tpm_tis_spi_main.c / tpm_tis_core.c flow:
 * - TIS register access over SPI with wait-state polling
 * - locality request/release and FIFO based command transport
 * - TPM2 HashSequenceStart/SequenceUpdate/SequenceComplete (SM3)
 */

#include <common.h>
#include <asm-generic/errno.h>
#include <environment.h>
#include <linux/ctype.h>
#include <spi.h>

#include <tpm_tis_spi.h>

#define TPM_TIS_SPI_FRAME_MAX            64
#define TPM_TIS_SPI_WAIT_RETRY           5000
#define TPM_TIS_SPI_TRANSFER_RETRY       3
#define TPM_TIS_SPI_DATA_DELAY_US_DEFAULT 5
#define TPM_TIS_SPI_DATA_DELAY_US_MAX     100
#define TPM_TIS_SPI_WAIT_TIMEOUT_MS      750
#define TPM_TIS_SPI_WAIT_POLL_US         10
#define TPM_TIS_SPI_TIS_POLL_US_DEFAULT      10
#define TPM_TIS_SPI_TIS_POLL_US_MAX          1000

#define TPM_TIS_SPI_TIMEOUT_LOCALITY_MS      1000
#define TPM_TIS_SPI_TIMEOUT_COMMAND_MS       2000
#define TPM_TIS_SPI_TIMEOUT_RESPONSE_MS      4000

#define TPM_TIS_SPI_HASH_CHUNK_DEFAULT       1024
#define TPM_TIS_SPI_HASH_CHUNK_MAX           4096
#define TPM_TIS_SPI_HASH_PROGRESS_STEP        (512 * 1024UL)

#define TPM_TIS_SPI_PROBE_RETRY_COUNT        3
#define TPM_TIS_SPI_PROBE_RETRY_DELAY_MS     20

/* TPM TIS register map (locality 0). */
#define TPM_ACCESS_0                   0x0000
#define TPM_STS_0                      0x0018
#define TPM_DATA_FIFO_0                0x0024
#define TPM_DID_VID_0                  0x0f00
#define TPM_RID_0                      0x0f04

/* TPM_ACCESS bits. */
#define TPM_ACCESS_VALID               0x80
#define TPM_ACCESS_ACTIVE_LOCALITY     0x20
#define TPM_ACCESS_REQUEST_USE         0x02

/* TPM_STS bits. */
#define TPM_STS_VALID                  0x80
#define TPM_STS_COMMAND_READY          0x40
#define TPM_STS_GO                     0x20
#define TPM_STS_DATA_AVAIL             0x10
#define TPM_STS_DATA_EXPECT            0x08

/* TPM2 constants used by this file. */
#define TPM2_ST_NO_SESSIONS            0x8001
#define TPM2_ST_SESSIONS               0x8002
#define TPM2_CC_HASH_SEQUENCE_START    0x00000186
#define TPM2_CC_SEQUENCE_UPDATE        0x0000015c
#define TPM2_CC_SEQUENCE_COMPLETE      0x0000013e
#define TPM2_CC_STARTUP                0x00000144
#define TPM2_CC_FLUSH_CONTEXT          0x00000165
#define TPM2_CC_NV_READ                0x0000014e
#define TPM2_CC_LOAD_EXTERNAL          0x00000167
#define TPM2_CC_VERIFY_SIGNATURE       0x00000177
#define TPM2_SU_CLEAR                  0x0000
#define TPM2_ALG_NULL                  0x0010
#define TPM2_ALG_SM3_256               0x0012
#define TPM2_ALG_SM2                   0x001b
#define TPM2_ALG_ECC                   0x0023
#define TPM2_ECC_SM2_P256              0x0020
#define TPM2_RH_OWNER                  0x40000001
#define TPM2_RS_PW                     0x40000009
#define TPM2_RH_NULL                   0x40000007
#define TPM2_RH_PLATFORM               0x4000000c
#define TPM2_RC_SUCCESS                0x00000000
#define TPM_RC_INITIALIZE              0x00000100
#define TPM2_RC_NV_UNDEFINED_1         0x0000018b
#define TPM2_RC_NV_UNDEFINED_0         0x0000028b
#define TPM2_RC_NV_UNDEFINED_2         0x0000098b
#define TPM2_RC_NV_UNDEFINED_3         0x00000b8b

#define TPMA_OBJECT_SENSITIVEDATAORIGIN 0x00000020
#define TPMA_OBJECT_USERWITHAUTH        0x00000040
#define TPMA_OBJECT_NODA                0x00000400
#define TPMA_OBJECT_SIGN                0x00040000
#define TPM_TIS_SPI_SM2_OBJECT_ATTRS          (TPMA_OBJECT_NODA | \
					 TPMA_OBJECT_USERWITHAUTH | \
					 TPMA_OBJECT_SIGN)

#define TPM_TIS_SPI_ENV_SPI_BUS              "tpm_tis_spi_bus"
#define TPM_TIS_SPI_ENV_SPI_CS               "tpm_tis_spi_cs"
#define TPM_TIS_SPI_ENV_SPI_HZ               "tpm_tis_spi_hz"
#define TPM_TIS_SPI_ENV_SPI_MODE             "tpm_tis_spi_mode"
#define TPM_TIS_SPI_ENV_SPI_DELAY_US         "tpm_tis_spi_delay_us"
#define TPM_TIS_SPI_ENV_HASH_CHUNK           "tpm_tis_spi_hash_chunk"
#define TPM_TIS_SPI_ENV_TIS_POLL_US          "tpm_tis_spi_tis_poll_us"
#define TPM_TIS_SPI_ENV_PROFILE              "tpm_tis_spi_profile"
#define TPM_TIS_SPI_ENV_VERIFY_ENABLE        "tpm_tis_spi_verify"
#define TPM_TIS_SPI_ENV_VERIFY_DIGEST        "tpm_tis_spi_kernel_digest"

#define TPM_TIS_SPI_POLICY_PUBKEY_OFFSET      0x004c
#define TPM_TIS_SPI_POLICY_PUBKEY_SIZE        65

struct tpm_tis_spi_context {
	struct spi_slave *slave;
	unsigned int bus;
	unsigned int cs;
	unsigned int hz;
	unsigned int mode;
	int opened;
};

struct tpm_tis_spi_submit_profile {
	ulong count;
	ulong command_ready_ms;
	ulong write_fifo_ms;
	ulong sts_go_ms;
	ulong wait_data_avail_ms;
	ulong read_rsp_header_ms;
	ulong read_rsp_body_ms;
	ulong rsp_parse_ms;
	ulong total_ms;
};

static struct tpm_tis_spi_context g_tpm_tis_spi_ctx;
static struct tpm_tis_spi_submit_profile g_tpm_tis_spi_update_profile;


/*
 * 功能：读取 U-Boot 环境变量并解析为 u32；若未设置则返回默认值。
 */
static unsigned int tpm_tis_spi_env_u32(const char *name, unsigned int dflt)
{
	const char *value = getenv(name);

	if (!value || !value[0])
		return dflt;

	return (unsigned int)simple_strtoul(value, NULL, 0);
}

/*
 * 功能：把环境变量解析为开关量，支持 0/n/f 表示关闭，其余视为开启。
 */
static int tpm_tis_spi_env_enabled(const char *name, int dflt)
{
	const char *value = getenv(name);

	if (!value || !value[0])
		return dflt;

	if ((value[0] == '0') || (value[0] == 'n') || (value[0] == 'N') ||
	    (value[0] == 'f') || (value[0] == 'F'))
		return 0;

	return 1;
}

/*
 * 功能：读取 TIS 状态轮询间隔，默认 10us，便于优化大数据度量耗时。
 */
static unsigned int tpm_tis_spi_tis_poll_us(void)
{
	unsigned int poll_us;

	poll_us = tpm_tis_spi_env_u32(TPM_TIS_SPI_ENV_TIS_POLL_US,
				 TPM_TIS_SPI_TIS_POLL_US_DEFAULT);
	if (!poll_us)
		poll_us = TPM_TIS_SPI_TIS_POLL_US_DEFAULT;
	if (poll_us > TPM_TIS_SPI_TIS_POLL_US_MAX)
		poll_us = TPM_TIS_SPI_TIS_POLL_US_MAX;

	return poll_us;
}

/*
 * 功能：统一输出失败阶段，便于定位超时发生点。
 */
static void tpm_tis_spi_stage_fail(const char *who, const char *stage, int rc)
{
	printf("TPM_TIS_SPI[%s] %s failed: %d\n", who, stage, rc);
}

/*
 * 功能：将 16 位整数按大端格式写入缓冲区。
 */
static void tpm_tis_spi_put_u16_be(u8 *buf, u16 value)
{
	buf[0] = (u8)((value >> 8) & 0xff);
	buf[1] = (u8)(value & 0xff);
}

/*
 * 功能：将 32 位整数按大端格式写入缓冲区。
 */
static void tpm_tis_spi_put_u32_be(u8 *buf, u32 value)
{
	buf[0] = (u8)((value >> 24) & 0xff);
	buf[1] = (u8)((value >> 16) & 0xff);
	buf[2] = (u8)((value >> 8) & 0xff);
	buf[3] = (u8)(value & 0xff);
}

/*
 * 功能：从缓冲区按大端格式读取 16 位整数。
 */
static u16 tpm_tis_spi_get_u16_be(const u8 *buf)
{
	return (u16)(((u16)buf[0] << 8) | buf[1]);
}

/*
 * 功能：从缓冲区按大端格式读取 32 位整数。
 */
static u32 tpm_tis_spi_get_u32_be(const u8 *buf)
{
	return ((u32)buf[0] << 24) | ((u32)buf[1] << 16) |
		((u32)buf[2] << 8) | buf[3];
}

/*
 * 功能：关闭 SPI 会话并释放总线/设备资源。
 */
static void tpm_tis_spi_close(struct tpm_tis_spi_context *ctx)
{
	if (!ctx->opened)
		return;

	spi_release_bus(ctx->slave);
	spi_free_slave(ctx->slave);
	ctx->slave = NULL;
	ctx->opened = 0;
}

/*
 * 功能：按环境变量参数打开 SPI 从设备并申请总线。
 */
static int tpm_tis_spi_open(struct tpm_tis_spi_context *ctx)
{
	struct spi_slave *slave;
	int rc;

	if (ctx->opened)
		return 0;

	ctx->bus = tpm_tis_spi_env_u32(TPM_TIS_SPI_ENV_SPI_BUS, 0);
	ctx->cs = tpm_tis_spi_env_u32(TPM_TIS_SPI_ENV_SPI_CS, 0);
	ctx->hz = tpm_tis_spi_env_u32(TPM_TIS_SPI_ENV_SPI_HZ, 25000000); // 25Mhz
	ctx->mode = tpm_tis_spi_env_u32(TPM_TIS_SPI_ENV_SPI_MODE, SPI_MODE_0);

	slave = spi_setup_slave(ctx->bus, ctx->cs, ctx->hz, ctx->mode);
	if (!slave) {
		printf("TPM_TIS_SPI: spi_setup_slave failed (bus=%u cs=%u)\n",
		       ctx->bus, ctx->cs);
		return -ENODEV;
	}

	rc = spi_claim_bus(slave);
	if (rc) {
		printf("TPM_TIS_SPI: spi_claim_bus failed: %d\n", rc);
		spi_free_slave(slave);
		return rc;
	}

	ctx->slave = slave;
	ctx->opened = 1;
	return 0;
}

/*
 * 功能：轮询 SPI wait-state 就绪位，确保后续数据阶段可访问。
 */
static int tpm_tis_spi_wait_ready(struct tpm_tis_spi_context *ctx, u8 header_last)
{
	u8 rx = 0;
	ulong start;
	int rc;

	if (header_last & 0x01)
		return 0;

	/*
	 * wait-state 改为按“时间”而非固定次数轮询，避免提升 SPI 频率后
	 * 实际等待窗口变短，导致同一硬件在高频下更容易超时。
	 */
	start = get_timer(0);
	while (get_timer(start) < TPM_TIS_SPI_WAIT_TIMEOUT_MS) {
		rc = spi_xfer(ctx->slave, 8, NULL, &rx, 0);
		if (rc) {
			printf("[wait_ready] spi_xfer failed rc=%d last_rx=0x%02x\n",
					rc, header_last);
			return rc;
		}
		if (rx & 0x01)
			return 0;
		udelay(TPM_TIS_SPI_WAIT_POLL_US);
	}

	printf("[wait_ready] ready bit timeout last_rx=0x%02x\n", header_last);
	return -ETIMEDOUT;
}

/*
 * 功能：异常路径下强制结束一次 SPI 事务，避免 CS 维持导致后续超时。
 */
static void tpm_tis_spi_force_end(struct tpm_tis_spi_context *ctx)
{
	if (!ctx || !ctx->slave)
		return;

	/* bitlen=0 仅触发 SPI_XFER_END，不额外在总线上发送数据。 */
	(void)spi_xfer(ctx->slave, 0, NULL, NULL, SPI_XFER_END);
}

/*
 * 功能：执行一次 TIS 寄存器读写传输，支持分帧与失败重试。
 */
static int tpm_tis_spi_tis_transfer(struct tpm_tis_spi_context *ctx, u16 reg,
			      u8 *in, const u8 *out, u16 len)
{
	u8 header[4];
	u8 dummy[TPM_TIS_SPI_FRAME_MAX];
	u8 rbuf[TPM_TIS_SPI_FRAME_MAX];
	u16 done = 0;
	u16 frame_len;
	u16 xfer_reg;
	unsigned int xfer_delay_us;
	int rc;
	int retry;
	const char *fail_stage;

	memset(dummy, 0, sizeof(dummy));

	xfer_delay_us = tpm_tis_spi_env_u32(TPM_TIS_SPI_ENV_SPI_DELAY_US,
				 TPM_TIS_SPI_DATA_DELAY_US_DEFAULT);
	if (xfer_delay_us > TPM_TIS_SPI_DATA_DELAY_US_MAX)
		xfer_delay_us = TPM_TIS_SPI_DATA_DELAY_US_MAX;

	while (done < len) {
		frame_len = len - done;
		if (frame_len > TPM_TIS_SPI_FRAME_MAX)
			frame_len = TPM_TIS_SPI_FRAME_MAX;

		xfer_reg = reg;

		/*
		 * FIFO(0x24) 分帧时地址必须保持不变；
		 * 其它寄存器按地址递增，兼容多字节寄存器读写。
		 */
		if (reg != TPM_DATA_FIFO_0)
			xfer_reg = reg + done;

		header[0] = (in ? 0x80 : 0x00) | (u8)(frame_len - 1);
		header[1] = 0xd4;
		header[2] = (u8)(xfer_reg >> 8);
		header[3] = (u8)(xfer_reg & 0xff);

		rc = -EIO;
		for (retry = 0; retry < TPM_TIS_SPI_TRANSFER_RETRY; retry++) {
			fail_stage = "header";
			rc = spi_xfer(ctx->slave, 32, header, header, SPI_XFER_BEGIN);
			if (rc) {
				tpm_tis_spi_force_end(ctx);
				continue;
			}

			fail_stage = "wait_ready";
			rc = tpm_tis_spi_wait_ready(ctx, header[3]);
			if (rc) {
				tpm_tis_spi_force_end(ctx);
				continue;
			}

			/*
			 * 参考 Linux SPI TIS 路径，头阶段后保留微小时序间隔，
			 * 改善较高 SPI 频率下的稳定性。
			 */
			if (xfer_delay_us)
				udelay(xfer_delay_us);

			if (in) {
				fail_stage = "data_read";
				rc = spi_xfer(ctx->slave, frame_len * 8,
					      dummy, rbuf, SPI_XFER_END);
				if (!rc)
					memcpy(in + done, rbuf, frame_len);
			} else {
				fail_stage = "data_write";
				rc = spi_xfer(ctx->slave, frame_len * 8,
					      out + done, rbuf, SPI_XFER_END);
			}
			if (!rc)
				break;

			/* data 阶段失败时也兜底结束事务，防止 CS 遗留。 */
			tpm_tis_spi_force_end(ctx);
		}
		if (rc) {
			tpm_tis_spi_force_end(ctx);
			printf("TPM_TIS_SPI[tis_xfer] %s reg=0x%04x off=%u frame=%u stage=%s rc=%d\n",
			       in ? "read" : "write", (unsigned int)xfer_reg,
			       (unsigned int)done, (unsigned int)frame_len,
			       fail_stage ? fail_stage : "unknown", rc);
			return rc;
		}

		done += frame_len;
	}

	return 0;
}

/*
 * 功能：从指定 TIS 寄存器读取多字节数据。
 */
static int tpm_tis_spi_tis_read(struct tpm_tis_spi_context *ctx, u16 reg, u8 *buf, u16 len)
{
	return tpm_tis_spi_tis_transfer(ctx, reg, buf, NULL, len);
}

/*
 * 功能：向指定 TIS 寄存器写入多字节数据。
 */
static int tpm_tis_spi_tis_write(struct tpm_tis_spi_context *ctx, u16 reg,
			   const u8 *buf, u16 len)
{
	return tpm_tis_spi_tis_transfer(ctx, reg, NULL, buf, len);
}

/*
 * 功能：从指定 TIS 寄存器读取 1 字节。
 */
static int tpm_tis_spi_tis_read8(struct tpm_tis_spi_context *ctx, u16 reg, u8 *value)
{
	return tpm_tis_spi_tis_read(ctx, reg, value, 1);
}

/*
 * 功能：向指定 TIS 寄存器写入 1 字节。
 */
static int tpm_tis_spi_tis_write8(struct tpm_tis_spi_context *ctx, u16 reg, u8 value)
{
	return tpm_tis_spi_tis_write(ctx, reg, &value, 1);
}

/*
 * 功能：等待 TPM_STS 达到期望状态位，带毫秒级超时。
 */
static int tpm_tis_spi_tis_wait_for_status(struct tpm_tis_spi_context *ctx,
				     u8 mask, u8 expected, ulong timeout_ms)
{
	ulong start;
	unsigned int poll_us;
	u8 status;
	int rc;

	poll_us = tpm_tis_spi_tis_poll_us();
	start = get_timer(0);
	while (get_timer(start) < timeout_ms) {
		rc = tpm_tis_spi_tis_read8(ctx, TPM_STS_0, &status);
		if (rc)
			return rc;
		if ((status & mask) == expected)
			return 0;
		udelay(poll_us);
	}

	return -ETIMEDOUT;
}

/*
 * 功能：读取 TIS burstcount，用于 FIFO 分块发送/接收。
 */
static int tpm_tis_spi_tis_get_burstcount(struct tpm_tis_spi_context *ctx)
{
	u8 sts[3];
	u16 burst;
	ulong start;
	unsigned int poll_us;
	int rc;

	poll_us = tpm_tis_spi_tis_poll_us();
	start = get_timer(0);
	while (get_timer(start) < TPM_TIS_SPI_TIMEOUT_COMMAND_MS) {
		rc = tpm_tis_spi_tis_read(ctx, TPM_STS_0, sts, sizeof(sts));
		if (rc)
			return rc;
		burst = ((u16)sts[2] << 8) | sts[1];
		if (burst)
			return burst;
		udelay(poll_us);
	}

	return -ETIMEDOUT;
}

/*
 * 功能：申请 locality 0 的访问权。
 */
static int tpm_tis_spi_tis_request_locality(struct tpm_tis_spi_context *ctx)
{
	u8 access;
	ulong start;
	unsigned int poll_us;
	int rc;

	poll_us = tpm_tis_spi_tis_poll_us();
	rc = tpm_tis_spi_tis_write8(ctx, TPM_ACCESS_0, TPM_ACCESS_REQUEST_USE);
	if (rc)
		return rc;

	start = get_timer(0);
	while (get_timer(start) < TPM_TIS_SPI_TIMEOUT_LOCALITY_MS) {
		rc = tpm_tis_spi_tis_read8(ctx, TPM_ACCESS_0, &access);
		if (rc)
			return rc;
		if ((access & (TPM_ACCESS_ACTIVE_LOCALITY | TPM_ACCESS_VALID)) ==
		    (TPM_ACCESS_ACTIVE_LOCALITY | TPM_ACCESS_VALID))
			return 0;
		udelay(poll_us);
	}

	return -ETIMEDOUT;
}

/*
 * 功能：释放 locality 0 的访问权。
 */
static int tpm_tis_spi_tis_release_locality(struct tpm_tis_spi_context *ctx)
{
	return tpm_tis_spi_tis_write8(ctx, TPM_ACCESS_0, TPM_ACCESS_ACTIVE_LOCALITY);
}

/*
 * 功能：将 TPM 状态切换到 COMMAND_READY。
 */
static int tpm_tis_spi_tis_command_ready(struct tpm_tis_spi_context *ctx)
{
	int rc;

	rc = tpm_tis_spi_tis_write8(ctx, TPM_STS_0, TPM_STS_COMMAND_READY);
	if (rc)
		return rc;

	/*
	 * 与 Linux tpm_tis_core 对齐：这里只等待 COMMAND_READY。
	 * 部分器件在该阶段不会稳定置位 STS_VALID，强依赖 0xC0 会导致超时。
	 */
	return tpm_tis_spi_tis_wait_for_status(ctx,
				 TPM_STS_COMMAND_READY,
				 TPM_STS_COMMAND_READY,
				 TPM_TIS_SPI_TIMEOUT_COMMAND_MS);
}

/*
 * 功能：按 burstcount 分块把命令体写入 TPM FIFO。
 */
static int tpm_tis_spi_tis_write_fifo(struct tpm_tis_spi_context *ctx,
					const u8 *buf, u32 len)
{
	u32 sent = 0;
	u16 chunk;
	int burst;
	int rc;
	u8 status;

	if (!len)
		return -EINVAL;

	/*
	 * Send all but the last byte in burst-sized chunks. The TPM should
	 * keep DATA_EXPECT asserted until the final byte is accepted.
	 */
	while (sent + 1 < len) {
		burst = tpm_tis_spi_tis_get_burstcount(ctx);
		if (burst < 0)
			return burst;
		chunk = (u16)(len - sent - 1);
		if (chunk > (u16)burst)
			chunk = (u16)burst;

		rc = tpm_tis_spi_tis_write(ctx, TPM_DATA_FIFO_0, buf + sent, chunk);
		if (rc)
			return rc;
		sent += chunk;

		rc = tpm_tis_spi_tis_wait_for_status(ctx, TPM_STS_VALID,
					 TPM_STS_VALID,
					 TPM_TIS_SPI_TIMEOUT_COMMAND_MS);
		if (rc)
			return rc;
		rc = tpm_tis_spi_tis_read8(ctx, TPM_STS_0, &status);
		if (rc)
			return rc;
		if ((status & TPM_STS_DATA_EXPECT) == 0)
			return -EIO;
	}

	/* Write the final byte and verify DATA_EXPECT gets cleared. */
	rc = tpm_tis_spi_tis_write8(ctx, TPM_DATA_FIFO_0, buf[sent]);
	if (rc)
		return rc;

	rc = tpm_tis_spi_tis_wait_for_status(ctx, TPM_STS_VALID,
					 TPM_STS_VALID,
					 TPM_TIS_SPI_TIMEOUT_COMMAND_MS);
	if (rc)
		return rc;
	rc = tpm_tis_spi_tis_read8(ctx, TPM_STS_0, &status);
	if (rc)
		return rc;
	if (status & TPM_STS_DATA_EXPECT)
		return -EIO;

	return 0;
}

/*
 * 功能：按 burstcount 分块从 TPM FIFO 读取响应体。
 */
static int tpm_tis_spi_tis_read_fifo(struct tpm_tis_spi_context *ctx, u8 *buf, u32 len)
{
	u32 got = 0;
	u16 chunk;
	int burst;
	int rc;

	while (got < len) {
		rc = tpm_tis_spi_tis_wait_for_status(ctx,
					 TPM_STS_DATA_AVAIL | TPM_STS_VALID,
					 TPM_STS_DATA_AVAIL | TPM_STS_VALID,
					 TPM_TIS_SPI_TIMEOUT_RESPONSE_MS);
		if (rc)
			return rc;

		burst = tpm_tis_spi_tis_get_burstcount(ctx);
		if (burst < 0)
			return burst;
		chunk = (u16)(len - got);
		if (chunk > (u16)burst)
			chunk = (u16)burst;

		rc = tpm_tis_spi_tis_read(ctx, TPM_DATA_FIFO_0, buf + got, chunk);
		if (rc)
			return rc;
		got += chunk;
	}

	return 0;
}

/*
 * 功能：通过 TIS FIFO 发送 TPM2 命令并收取响应。
 */
static int tpm_tis_spi_submit(struct tpm_tis_spi_context *ctx,
			     const u8 *cmd, u32 cmd_len,
			     u8 *rsp, u32 *rsp_len, u32 rsp_max)
{
	u8 header[10];
	u32 total_len;
	u32 rc_code;
	u32 cc = 0;
	unsigned int profile;
	int is_update = 0;
	ulong submit_start = 0;
	ulong stage_start = 0;
	ulong command_ready_ms = 0;
	ulong write_fifo_ms = 0;
	ulong sts_go_ms = 0;
	ulong wait_data_avail_ms = 0;
	ulong read_rsp_header_ms = 0;
	ulong read_rsp_body_ms = 0;
	ulong rsp_parse_ms = 0;
	int rc;

	if (cmd_len >= 10)
		cc = tpm_tis_spi_get_u32_be(&cmd[6]);
	profile = tpm_tis_spi_env_u32(TPM_TIS_SPI_ENV_PROFILE, 0);
	is_update = profile && (cc == TPM2_CC_SEQUENCE_UPDATE);
	if (is_update)
		submit_start = get_timer(0);

	if (is_update)
		stage_start = get_timer(0);
	rc = tpm_tis_spi_tis_command_ready(ctx);
	if (is_update)
		command_ready_ms = get_timer(stage_start);
	if (rc) {
		tpm_tis_spi_stage_fail("submit", "command_ready", rc);
		goto out;
	}

	if (is_update)
		stage_start = get_timer(0);
	rc = tpm_tis_spi_tis_write_fifo(ctx, cmd, cmd_len);
	if (is_update)
		write_fifo_ms = get_timer(stage_start);
	if (rc) {
		tpm_tis_spi_stage_fail("submit", "write_fifo", rc);
		goto out;
	}

	if (is_update)
		stage_start = get_timer(0);
	rc = tpm_tis_spi_tis_write8(ctx, TPM_STS_0, TPM_STS_GO);
	if (is_update)
		sts_go_ms = get_timer(stage_start);
	if (rc) {
		tpm_tis_spi_stage_fail("submit", "sts_go", rc);
		goto out;
	}

	if (is_update)
		stage_start = get_timer(0);
	rc = tpm_tis_spi_tis_wait_for_status(ctx,
				 TPM_STS_DATA_AVAIL | TPM_STS_VALID,
				 TPM_STS_DATA_AVAIL | TPM_STS_VALID,
				 TPM_TIS_SPI_TIMEOUT_RESPONSE_MS);
	if (is_update)
		wait_data_avail_ms = get_timer(stage_start);
	if (rc) {
		tpm_tis_spi_stage_fail("submit", "wait_data_avail", rc);
		goto out;
	}

	if (is_update)
		stage_start = get_timer(0);
	rc = tpm_tis_spi_tis_read_fifo(ctx, header, sizeof(header));
	if (is_update)
		read_rsp_header_ms = get_timer(stage_start);
	if (rc) {
		tpm_tis_spi_stage_fail("submit", "read_rsp_header", rc);
		goto out;
	}

	if (is_update)
		stage_start = get_timer(0);
	total_len = tpm_tis_spi_get_u32_be(&header[2]);
	if (total_len < sizeof(header) || total_len > rsp_max) {
		rc = -EMSGSIZE;
		tpm_tis_spi_stage_fail("submit", "rsp_size", rc);
		if (is_update)
			rsp_parse_ms = get_timer(stage_start);
		goto out;
	}

	memcpy(rsp, header, sizeof(header));
	if (is_update)
		rsp_parse_ms = get_timer(stage_start);

	if (total_len > sizeof(header)) {
		if (is_update)
			stage_start = get_timer(0);
		rc = tpm_tis_spi_tis_read_fifo(ctx, rsp + sizeof(header),
					total_len - sizeof(header));
		if (is_update)
			read_rsp_body_ms = get_timer(stage_start);
		if (rc)
			goto out;
	}

	if (is_update)
		stage_start = get_timer(0);
	*rsp_len = total_len;
	rc_code = tpm_tis_spi_get_u32_be(&rsp[6]);
	if (rc_code != TPM2_RC_SUCCESS) {
		if ((cc == TPM2_CC_STARTUP) && (rc_code == TPM_RC_INITIALIZE)) {
			printf("TPM_TIS_SPI: TPM2_Startup already initialized, continue (rc=0x%08x)\n",
			       rc_code);
			rc = 0;
			if (is_update)
				rsp_parse_ms += get_timer(stage_start);
			goto out;
		}
		printf("TPM_TIS_SPI: TPM command failed cc=0x%08x rc=0x%08x\n", cc, rc_code);
		if (cc == TPM2_CC_NV_READ &&
		    (rc_code == TPM2_RC_NV_UNDEFINED_1 ||
		     rc_code == TPM2_RC_NV_UNDEFINED_0 ||
		     rc_code == TPM2_RC_NV_UNDEFINED_2 ||
		     rc_code == TPM2_RC_NV_UNDEFINED_3))
			rc = -ENOENT;
		else
			rc = -EACCES;
		if (is_update)
			rsp_parse_ms += get_timer(stage_start);
		goto out;
	}
	if (is_update)
		rsp_parse_ms += get_timer(stage_start);

	rc = 0;
out:
	if (is_update) {
		g_tpm_tis_spi_update_profile.count++;
		g_tpm_tis_spi_update_profile.command_ready_ms += command_ready_ms;
		g_tpm_tis_spi_update_profile.write_fifo_ms += write_fifo_ms;
		g_tpm_tis_spi_update_profile.sts_go_ms += sts_go_ms;
		g_tpm_tis_spi_update_profile.wait_data_avail_ms += wait_data_avail_ms;
		g_tpm_tis_spi_update_profile.read_rsp_header_ms += read_rsp_header_ms;
		g_tpm_tis_spi_update_profile.read_rsp_body_ms += read_rsp_body_ms;
		g_tpm_tis_spi_update_profile.rsp_parse_ms += rsp_parse_ms;
		g_tpm_tis_spi_update_profile.total_ms += get_timer(submit_start);
	}
	return rc;
}


/*
 * 功能：发送 TPM2_Startup(TPM2_SU_CLEAR) 进行初始化。
 */
static int tpm_tis_spi_startup(struct tpm_tis_spi_context *ctx)
{
	u8 cmd[12];
	u8 rsp[64];
	u32 rsp_len;

	tpm_tis_spi_put_u16_be(&cmd[0], TPM2_ST_NO_SESSIONS);
	tpm_tis_spi_put_u32_be(&cmd[2], sizeof(cmd));
	tpm_tis_spi_put_u32_be(&cmd[6], TPM2_CC_STARTUP);
	tpm_tis_spi_put_u16_be(&cmd[10], TPM2_SU_CLEAR);

	/* 仅在 TPM_RC_INITIALIZE 时由 submit 内部放行，其余错误需上抛。 */
	return tpm_tis_spi_submit(ctx, cmd, sizeof(cmd), rsp, &rsp_len, sizeof(rsp));
}



/*
 * 功能：在哈希序列异常时回收 sequence 句柄，避免资源泄漏。
 */
static int tpm_tis_spi_flush_context(struct tpm_tis_spi_context *ctx, u32 handle)
{
	u8 cmd[14];
	u8 rsp[32];
	u32 rsp_len;

	tpm_tis_spi_put_u16_be(&cmd[0], TPM2_ST_NO_SESSIONS);
	tpm_tis_spi_put_u32_be(&cmd[2], sizeof(cmd));
	tpm_tis_spi_put_u32_be(&cmd[6], TPM2_CC_FLUSH_CONTEXT);
	tpm_tis_spi_put_u32_be(&cmd[10], handle);

	return tpm_tis_spi_submit(ctx, cmd, sizeof(cmd), rsp, &rsp_len, sizeof(rsp));
}

/*
 * 功能：发送 HashSequenceStart，创建 SM3 序列句柄。
 */
static int tpm_tis_spi_hashseq_start_sm3(struct tpm_tis_spi_context *ctx, u32 *seq)
{
	u8 cmd[14];
	u8 rsp[64];
	u32 rsp_len;
	int rc;

	tpm_tis_spi_put_u16_be(&cmd[0], TPM2_ST_NO_SESSIONS);
	tpm_tis_spi_put_u32_be(&cmd[2], sizeof(cmd));
	tpm_tis_spi_put_u32_be(&cmd[6], TPM2_CC_HASH_SEQUENCE_START);
	tpm_tis_spi_put_u16_be(&cmd[10], 0x0000); /* empty auth */
	tpm_tis_spi_put_u16_be(&cmd[12], TPM2_ALG_SM3_256);

	rc = tpm_tis_spi_submit(ctx, cmd, sizeof(cmd), rsp, &rsp_len, sizeof(rsp));
	if (rc)
		return rc;
	if (rsp_len < 14)
		return -EPROTO;

	*seq = tpm_tis_spi_get_u32_be(&rsp[10]);
	return 0;
}

/*
 * 功能：向序列句柄持续喂入一段待哈希数据。
 */
static int tpm_tis_spi_sequence_update(struct tpm_tis_spi_context *ctx,
				      u32 seq, const u8 *data, u16 data_len)
{
	u8 cmd[64 + TPM_TIS_SPI_HASH_CHUNK_MAX];
	u8 rsp[64];
	u32 rsp_len;
	u32 cmd_len;
	u32 off = 0;
	int rc;

	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ST_SESSIONS);
	off += 2;
	off += 4; /* commandSize */
	tpm_tis_spi_put_u32_be(&cmd[off], TPM2_CC_SEQUENCE_UPDATE);
	off += 4;
	tpm_tis_spi_put_u32_be(&cmd[off], seq);
	off += 4;

	/* authSize + one password session with empty auth. */
	tpm_tis_spi_put_u32_be(&cmd[off], 9);
	off += 4;
	tpm_tis_spi_put_u32_be(&cmd[off], TPM2_RS_PW);
	off += 4;
	tpm_tis_spi_put_u16_be(&cmd[off], 0);
	off += 2;
	cmd[off++] = 0x00;
	tpm_tis_spi_put_u16_be(&cmd[off], 0);
	off += 2;

	tpm_tis_spi_put_u16_be(&cmd[off], data_len);
	off += 2;
	if (data_len)
		memcpy(&cmd[off], data, data_len);
	off += data_len;

	cmd_len = off;
	tpm_tis_spi_put_u32_be(&cmd[2], cmd_len);

	rc = tpm_tis_spi_submit(ctx, cmd, cmd_len, rsp, &rsp_len, sizeof(rsp));
	if (rc)
		return rc;

	if (rsp_len < 10)
		return -EPROTO;

	return 0;
}

/*
 * 功能：结束哈希序列并提取最终 digest。
 */
static int tpm_tis_spi_sequence_complete(struct tpm_tis_spi_context *ctx,
					u32 seq, u8 digest[TPM_TIS_SPI_DIGEST_SIZE])
{
	u8 cmd[64];
	u8 rsp[256];
	u32 rsp_len;
	u32 parameter_offset;
	u16 digest_len;
	u32 cmd_len;
	u32 off = 0;
	int rc;

	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ST_SESSIONS);
	off += 2;
	off += 4; /* commandSize */
	tpm_tis_spi_put_u32_be(&cmd[off], TPM2_CC_SEQUENCE_COMPLETE);
	off += 4;
	tpm_tis_spi_put_u32_be(&cmd[off], seq);
	off += 4;

	tpm_tis_spi_put_u32_be(&cmd[off], 9);
	off += 4;
	tpm_tis_spi_put_u32_be(&cmd[off], TPM2_RS_PW);
	off += 4;
	tpm_tis_spi_put_u16_be(&cmd[off], 0);
	off += 2;
	cmd[off++] = 0x00;
	tpm_tis_spi_put_u16_be(&cmd[off], 0);
	off += 2;

	tpm_tis_spi_put_u16_be(&cmd[off], 0x0000); /* empty final buffer */
	off += 2;
	tpm_tis_spi_put_u32_be(&cmd[off], TPM2_RH_NULL);
	off += 4;

	cmd_len = off;
	tpm_tis_spi_put_u32_be(&cmd[2], cmd_len);

	rc = tpm_tis_spi_submit(ctx, cmd, cmd_len, rsp, &rsp_len, sizeof(rsp));
	if (rc)
		return rc;
	if (rsp_len < 16)
		return -EPROTO;

	if (tpm_tis_spi_get_u16_be(&rsp[0]) != TPM2_ST_SESSIONS)
		return -EPROTO;

	parameter_offset = 10 + 4; /* response header + parameterSize */
	if (rsp_len < parameter_offset + 2)
		return -EPROTO;

	digest_len = tpm_tis_spi_get_u16_be(&rsp[parameter_offset]);
	if (digest_len != TPM_TIS_SPI_DIGEST_SIZE)
		return -EPROTO;
	if (rsp_len < parameter_offset + 2 + digest_len)
		return -EPROTO;

	memcpy(digest, &rsp[parameter_offset + 2], digest_len);
	return 0;
}


/*
 * 功能：通过 TPM2_NV_Read 读取 NV 指定偏移的数据，当前用于读取策略公钥。
 */
static int tpm_tis_spi_nv_read(struct tpm_tis_spi_context *ctx, u32 auth_handle,
			      u32 nv_index, u16 offset_in, u8 *data,
			      u16 data_len)
{
	u8 cmd[64];
	u8 rsp[128];
	u32 rsp_len;
	u32 parameter_offset;
	u32 off = 0;
	u16 out_len;
	int rc;

	if (!data || !data_len)
		return -EINVAL;

	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ST_SESSIONS);
	off += 2;
	off += 4; /* commandSize */
	tpm_tis_spi_put_u32_be(&cmd[off], TPM2_CC_NV_READ);
	off += 4;

	/* handles: authHandle + nvIndex */
	tpm_tis_spi_put_u32_be(&cmd[off], auth_handle);
	off += 4;
	tpm_tis_spi_put_u32_be(&cmd[off], nv_index);
	off += 4;

	/* authSize + empty password session */
	tpm_tis_spi_put_u32_be(&cmd[off], 9);
	off += 4;
	tpm_tis_spi_put_u32_be(&cmd[off], TPM2_RS_PW);
	off += 4;
	tpm_tis_spi_put_u16_be(&cmd[off], 0);
	off += 2;
	cmd[off++] = 0x00;
	tpm_tis_spi_put_u16_be(&cmd[off], 0);
	off += 2;

	/* parameters: size + offset */
	tpm_tis_spi_put_u16_be(&cmd[off], data_len);
	off += 2;
	tpm_tis_spi_put_u16_be(&cmd[off], offset_in);
	off += 2;

	tpm_tis_spi_put_u32_be(&cmd[2], off);

	rc = tpm_tis_spi_submit(ctx, cmd, off, rsp, &rsp_len, sizeof(rsp));
	if (rc)
		return rc;
	if (rsp_len < 16)
		return -EPROTO;
	if (tpm_tis_spi_get_u16_be(&rsp[0]) != TPM2_ST_SESSIONS)
		return -EPROTO;

	parameter_offset = 10 + 4; /* response header + parameterSize */
	if (rsp_len < parameter_offset + 2)
		return -EPROTO;

	out_len = tpm_tis_spi_get_u16_be(&rsp[parameter_offset]);
	if (out_len != data_len) {
		printf("TPM_TIS_SPI[nvread] length mismatch index=0x%08x off=0x%x expected=%u actual=%u\n",
		       nv_index, offset_in, data_len, out_len);
		return -EPROTO;
	}
	if (rsp_len < parameter_offset + 2 + out_len)
		return -EPROTO;

	memcpy(data, &rsp[parameter_offset + 2], out_len);
	return 0;
}

/*
 * 功能：从策略 NV 中读取 pubkey[65]，转换为 VerifySignature 使用的 X||Y。
 */
static int tpm_tis_spi_read_policy_pubkey_xy(struct tpm_tis_spi_context *ctx,
					    u32 nv_index, u32 auth_handle,
					    u8 pubkey_xy[TPM_TIS_SPI_PUBKEY_XY_SIZE])
{
	u8 pubkey65[TPM_TIS_SPI_POLICY_PUBKEY_SIZE];
	int rc;

	rc = tpm_tis_spi_nv_read(ctx, auth_handle, nv_index,
				 TPM_TIS_SPI_POLICY_PUBKEY_OFFSET, pubkey65,
				 sizeof(pubkey65));
	if (rc)
		return rc;

	if (pubkey65[0] != 0x04) {
		printf("TPM_TIS_SPI[nvpub] bad pubkey prefix 0x%02x from NV 0x%08x\n",
		       pubkey65[0], nv_index);
		return -EINVAL;
	}

	memcpy(pubkey_xy, pubkey65 + 1, TPM_TIS_SPI_PUBKEY_XY_SIZE);
	printf("TPM_TIS_SPI[nvpub] use pubkey from NV 0x%08x offset 0x%04x\n",
	       nv_index, TPM_TIS_SPI_POLICY_PUBKEY_OFFSET);
	return 0;
}

/*
 * Classify TPM_TIS_SPI policy NV state for trustboot gating.
 *
 * Only an explicit NV-undefined response maps to
 * TPM_TIS_SPI_POLICY_NV_UNDEFINED. Transport, auth, and protocol errors remain
 * negative errors so upper layers do not treat them as first-time setup.
 */
int tpm_tis_spi_policy_nv_configured(u32 nv_index, u32 auth_handle)
{
	struct tpm_tis_spi_context *ctx = &g_tpm_tis_spi_ctx;
	u8 pubkey65[TPM_TIS_SPI_POLICY_PUBKEY_SIZE];
	int rc;

	rc = tpm_tis_spi_open(ctx);
	if (rc) {
		tpm_tis_spi_stage_fail("policy-nv", "spi_open", rc);
		return rc;
	}

	rc = tpm_tis_spi_tis_request_locality(ctx);
	if (rc) {
		tpm_tis_spi_stage_fail("policy-nv", "request_locality", rc);
		goto out;
	}

	rc = tpm_tis_spi_startup(ctx);
	if (rc) {
		tpm_tis_spi_stage_fail("policy-nv", "startup", rc);
		goto out_release;
	}

	rc = tpm_tis_spi_nv_read(ctx, auth_handle, nv_index,
				TPM_TIS_SPI_POLICY_PUBKEY_OFFSET, pubkey65,
				sizeof(pubkey65));
	if (rc == -ENOENT) {
		rc = TPM_TIS_SPI_POLICY_NV_UNDEFINED;
		goto out_release;
	}
	if (rc) {
		tpm_tis_spi_stage_fail("policy-nv", "read_nv_pubkey", rc);
		goto out_release;
	}

	if (pubkey65[0] != 0x04) {
		printf("TPM_TIS_SPI[policy-nv] bad pubkey prefix 0x%02x from NV 0x%08x\n",
		       pubkey65[0], nv_index);
		rc = TPM_TIS_SPI_POLICY_NV_INVALID;
		goto out_release;
	}

	rc = TPM_TIS_SPI_POLICY_NV_CONFIGURED;

out_release:
	(void)tpm_tis_spi_tis_release_locality(ctx);
out:
	tpm_tis_spi_close(ctx);
	return rc;
}


/*
 * 功能：构造 TPM2_LoadExternal 输入，把外部 SM2 公钥装载为临时对象。
 */
static int tpm_tis_spi_loadexternal_sm2(struct tpm_tis_spi_context *ctx,
				       const u8 pubkey_xy[TPM_TIS_SPI_PUBKEY_XY_SIZE],
				       u32 *key_handle)
{
	u8 cmd[192];
	u8 rsp[128];
	u32 rsp_len;
	u32 public_size;
	u32 public_start;
	u32 off = 0;
	int rc;

	if (!pubkey_xy || !key_handle)
		return -EINVAL;

	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ST_NO_SESSIONS);
	off += 2;
	off += 4; /* commandSize */
	tpm_tis_spi_put_u32_be(&cmd[off], TPM2_CC_LOAD_EXTERNAL);
	off += 4;

	/* inPrivate = empty TPM2B_SENSITIVE */
	tpm_tis_spi_put_u16_be(&cmd[off], 0);
	off += 2;

	/* inPublic.size placeholder */
	off += 2;
	public_start = off;

	/* TPMT_PUBLIC：外部公钥只用于验签，属性按 public-only signing key 设置。 */
	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ALG_ECC);
	off += 2;
	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ALG_SM3_256);
	off += 2;
	tpm_tis_spi_put_u32_be(&cmd[off], TPM_TIS_SPI_SM2_OBJECT_ATTRS);
	off += 4;
	tpm_tis_spi_put_u16_be(&cmd[off], 0); /* authPolicy */
	off += 2;

	/* TPMS_ECC_PARMS */
	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ALG_NULL); /* symmetric */
	off += 2;
	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ALG_SM2); /* scheme */
	off += 2;
	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ALG_SM3_256); /* scheme.hashAlg */
	off += 2;
	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ECC_SM2_P256); /* curveID */
	off += 2;
	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ALG_NULL); /* kdf */
	off += 2;

	/* unique.ecc.x/y */
	tpm_tis_spi_put_u16_be(&cmd[off], 32);
	off += 2;
	memcpy(&cmd[off], pubkey_xy, 32);
	off += 32;
	tpm_tis_spi_put_u16_be(&cmd[off], 32);
	off += 2;
	memcpy(&cmd[off], pubkey_xy + 32, 32);
	off += 32;

	public_size = off - public_start;
	tpm_tis_spi_put_u16_be(&cmd[public_start - 2], (u16)public_size);

	/* hierarchy = TPM_RH_NULL */
	tpm_tis_spi_put_u32_be(&cmd[off], TPM2_RH_NULL);
	off += 4;

	tpm_tis_spi_put_u32_be(&cmd[2], off);

	rc = tpm_tis_spi_submit(ctx, cmd, off, rsp, &rsp_len, sizeof(rsp));
	if (rc)
		return rc;
	if (rsp_len < 14)
		return -EPROTO;

	*key_handle = tpm_tis_spi_get_u32_be(&rsp[10]);
	return 0;
}

/*
 * 功能：通过 TPM2_VerifySignature 使用指定 key handle 验证 SM2 签名。
 */
static int tpm_tis_spi_verifysignature_sm2(struct tpm_tis_spi_context *ctx,
					  u32 key_handle,
					  const u8 digest[TPM_TIS_SPI_DIGEST_SIZE],
					  const u8 signature[TPM_TIS_SPI_SIGNATURE_SIZE])
{
	u8 cmd[160];
	u8 rsp[128];
	u32 rsp_len;
	u32 off = 0;

	if (!digest || !signature || !key_handle)
		return -EINVAL;

	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ST_NO_SESSIONS);
	off += 2;
	off += 4; /* commandSize */
	tpm_tis_spi_put_u32_be(&cmd[off], TPM2_CC_VERIFY_SIGNATURE);
	off += 4;
	tpm_tis_spi_put_u32_be(&cmd[off], key_handle);
	off += 4;

	/* digest */
	tpm_tis_spi_put_u16_be(&cmd[off], TPM_TIS_SPI_DIGEST_SIZE);
	off += 2;
	memcpy(&cmd[off], digest, TPM_TIS_SPI_DIGEST_SIZE);
	off += TPM_TIS_SPI_DIGEST_SIZE;

	/* TPMT_SIGNATURE: SM2 + SM3 + r||s */
	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ALG_SM2);
	off += 2;
	tpm_tis_spi_put_u16_be(&cmd[off], TPM2_ALG_SM3_256);
	off += 2;
	tpm_tis_spi_put_u16_be(&cmd[off], 32);
	off += 2;
	memcpy(&cmd[off], signature, 32);
	off += 32;
	tpm_tis_spi_put_u16_be(&cmd[off], 32);
	off += 2;
	memcpy(&cmd[off], signature + 32, 32);
	off += 32;

	tpm_tis_spi_put_u32_be(&cmd[2], off);

	return tpm_tis_spi_submit(ctx, cmd, off, rsp, &rsp_len, sizeof(rsp));
}

/*
 * 功能：使用 TPM_TIS_SPI 内部/持久化 key handle 验证 digest 的 SM2 签名。
 */
int tpm_tis_spi_sigverify_sm2_handle(u32 key_handle,
				     const u8 digest[TPM_TIS_SPI_DIGEST_SIZE],
				     const u8 signature[TPM_TIS_SPI_SIGNATURE_SIZE])
{
	struct tpm_tis_spi_context *ctx = &g_tpm_tis_spi_ctx;
	int rc;

	rc = tpm_tis_spi_open(ctx);
	if (rc) {
		tpm_tis_spi_stage_fail("sigverify", "spi_open", rc);
		return rc;
	}

	rc = tpm_tis_spi_tis_request_locality(ctx);
	if (rc) {
		tpm_tis_spi_stage_fail("sigverify", "request_locality", rc);
		goto out;
	}

	rc = tpm_tis_spi_startup(ctx);
	if (rc) {
		tpm_tis_spi_stage_fail("sigverify", "startup", rc);
		goto out_release;
	}

	rc = tpm_tis_spi_verifysignature_sm2(ctx, key_handle, digest, signature);
	if (rc)
		tpm_tis_spi_stage_fail("sigverify", "verify_signature", rc);

out_release:
	(void)tpm_tis_spi_tis_release_locality(ctx);
out:
	tpm_tis_spi_close(ctx);
	return rc;
}

/*
 * 功能：从外部传入公钥，LoadExternal 后再验证 digest 的 SM2 签名。
 */
int tpm_tis_spi_sigverify_sm2_external(const u8 digest[TPM_TIS_SPI_DIGEST_SIZE],
				       const u8 signature[TPM_TIS_SPI_SIGNATURE_SIZE],
				       const u8 pubkey_xy[TPM_TIS_SPI_PUBKEY_XY_SIZE])
{
	struct tpm_tis_spi_context *ctx = &g_tpm_tis_spi_ctx;
	u32 key_handle = 0;
	int rc;

	rc = tpm_tis_spi_open(ctx);
	if (rc) {
		tpm_tis_spi_stage_fail("sigverifyext", "spi_open", rc);
		return rc;
	}

	rc = tpm_tis_spi_tis_request_locality(ctx);
	if (rc) {
		tpm_tis_spi_stage_fail("sigverifyext", "request_locality", rc);
		goto out;
	}

	rc = tpm_tis_spi_startup(ctx);
	if (rc) {
		tpm_tis_spi_stage_fail("sigverifyext", "startup", rc);
		goto out_release;
	}

	rc = tpm_tis_spi_loadexternal_sm2(ctx, pubkey_xy, &key_handle);
	if (rc) {
		tpm_tis_spi_stage_fail("sigverifyext", "load_external", rc);
		goto out_release;
	}

	rc = tpm_tis_spi_verifysignature_sm2(ctx, key_handle, digest, signature);
	if (rc)
		tpm_tis_spi_stage_fail("sigverifyext", "verify_signature", rc);

	(void)tpm_tis_spi_flush_context(ctx, key_handle);

out_release:
	(void)tpm_tis_spi_tis_release_locality(ctx);
out:
	tpm_tis_spi_close(ctx);
	return rc;
}

/*
 * 功能：每次从策略 NV 读取公钥，再走 LoadExternal + VerifySignature 验签。
 */
int tpm_tis_spi_sigverify_sm2_policy_nv(u32 nv_index, u32 auth_handle,
				       const u8 digest[TPM_TIS_SPI_DIGEST_SIZE],
				       const u8 signature[TPM_TIS_SPI_SIGNATURE_SIZE])
{
	struct tpm_tis_spi_context *ctx = &g_tpm_tis_spi_ctx;
	u8 pubkey_xy[TPM_TIS_SPI_PUBKEY_XY_SIZE];
	u32 key_handle = 0;
	int rc;

	rc = tpm_tis_spi_open(ctx);
	if (rc) {
		tpm_tis_spi_stage_fail("sigverifynv", "spi_open", rc);
		return rc;
	}

	rc = tpm_tis_spi_tis_request_locality(ctx);
	if (rc) {
		tpm_tis_spi_stage_fail("sigverifynv", "request_locality", rc);
		goto out;
	}

	rc = tpm_tis_spi_startup(ctx);
	if (rc) {
		tpm_tis_spi_stage_fail("sigverifynv", "startup", rc);
		goto out_release;
	}

	rc = tpm_tis_spi_read_policy_pubkey_xy(ctx, nv_index, auth_handle,
					       pubkey_xy);
	if (rc) {
		printf("TPM_TIS_SPI[sigverifynv] public key read failed: nv=0x%08x auth=0x%08x off=0x%04x len=%u rc=%d\n",
		       nv_index, auth_handle, TPM_TIS_SPI_POLICY_PUBKEY_OFFSET,
		       TPM_TIS_SPI_POLICY_PUBKEY_SIZE, rc);
		tpm_tis_spi_stage_fail("sigverifynv", "read_nv_pubkey", rc);
		goto out_release;
	}

	rc = tpm_tis_spi_loadexternal_sm2(ctx, pubkey_xy, &key_handle);
	if (rc) {
		printf("TPM_TIS_SPI[sigverifynv] public key load failed after NV read: rc=%d\n",
		       rc);
		tpm_tis_spi_stage_fail("sigverifynv", "load_external", rc);
		goto out_release;
	}

	rc = tpm_tis_spi_verifysignature_sm2(ctx, key_handle, digest, signature);
	if (rc) {
		printf("TPM_TIS_SPI[sigverifynv] signature verify failed: digest/signature/public-key mismatch or unsupported signature, rc=%d\n",
		       rc);
		printf("TPM_TIS_SPI[sigverifynv] note: this command does not hash memory or compare memory digest; it verifies the supplied digest only\n");
		tpm_tis_spi_stage_fail("sigverifynv", "verify_signature", rc);
	}

	(void)tpm_tis_spi_flush_context(ctx, key_handle);

out_release:
	(void)tpm_tis_spi_tis_release_locality(ctx);
out:
	tpm_tis_spi_close(ctx);
	return rc;
}

/*
 * 功能：以十六进制打印 32 字节 digest。
 */
void tpm_tis_spi_print_digest(const u8 digest[TPM_TIS_SPI_DIGEST_SIZE])
{
	int i;

	for (i = 0; i < TPM_TIS_SPI_DIGEST_SIZE; i++)
		printf("%02x", digest[i]);
	printf("\n");
}

/*
 * 功能：把单个十六进制字符转换为数值。
 */
static int tpm_tis_spi_hex_to_val(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

/*
 * 功能：把 64 字符十六进制串解析为 32 字节 digest。
 */
static int tpm_tis_spi_parse_digest_hex(const char *hex, u8 out[TPM_TIS_SPI_DIGEST_SIZE])
{
	int hi;
	int lo;
	int i;

	if (!hex)
		return -EINVAL;

	for (i = 0; i < TPM_TIS_SPI_DIGEST_SIZE; i++) {
		hi = tpm_tis_spi_hex_to_val(hex[i * 2]);
		lo = tpm_tis_spi_hex_to_val(hex[i * 2 + 1]);
		if (hi < 0 || lo < 0)
			return -EINVAL;
		out[i] = (u8)((hi << 4) | lo);
	}

	if (hex[TPM_TIS_SPI_DIGEST_SIZE * 2] != '\0')
		return -EINVAL;

	return 0;
}

/*
 * 功能：探测 TPM DID/VID 与 RID，验证总线连通性。
 */
int tpm_tis_spi_probe(u32 *did_vid, u8 *rid)
{
	struct tpm_tis_spi_context *ctx = &g_tpm_tis_spi_ctx;
	u8 did_buf[4];
	u8 rid_buf = 0;
	int rc = -ETIMEDOUT;
	int attempt;

	for (attempt = 0; attempt < TPM_TIS_SPI_PROBE_RETRY_COUNT; attempt++) {
		rc = tpm_tis_spi_open(ctx);
		if (rc) {
			tpm_tis_spi_stage_fail("probe", "spi_open", rc);
			return rc;
		}

		rc = tpm_tis_spi_tis_read(ctx, TPM_DID_VID_0, did_buf, sizeof(did_buf));
		if (rc) {
			printf("TPM_TIS_SPI[probe] read_didvid failed (attempt=%d): %d\n",
			       attempt + 1, rc);
		} else {
			rc = tpm_tis_spi_tis_read(ctx, TPM_RID_0, &rid_buf, sizeof(rid_buf));
			if (rc)
				printf("TPM_TIS_SPI[probe] read_rid failed (attempt=%d): %d\n",
				       attempt + 1, rc);
		}

		tpm_tis_spi_close(ctx);
		if (!rc)
			break;

		/* 首次上电/异常恢复后给 SPI/TPM 留一点恢复时间。 */
		mdelay(TPM_TIS_SPI_PROBE_RETRY_DELAY_MS);
	}

	if (rc) {
		tpm_tis_spi_stage_fail("probe", "final", rc);
		return rc;
	}

	if (did_vid) {
		*did_vid = ((u32)did_buf[3] << 24) | ((u32)did_buf[2] << 16) |
			  ((u32)did_buf[1] << 8) | did_buf[0];
	}
	if (rid)
		*rid = rid_buf;

	return 0;
}

/*
 * 功能：对内存区域执行 TPM2 SM3 序列哈希。
 */
int tpm_tis_spi_hash_sm3_mem(ulong addr, ulong len, u8 digest[TPM_TIS_SPI_DIGEST_SIZE])
{
	struct tpm_tis_spi_context *ctx = &g_tpm_tis_spi_ctx;
	const u8 *data;
	const u8 *data_cur;
	ulong remaining;
	ulong processed = 0;
	ulong next_progress = TPM_TIS_SPI_HASH_PROGRESS_STEP;
	u32 seq = 0;
	u32 chunk = 0;
	unsigned int profile;
	ulong total_start = 0;
	ulong stage_start = 0;
	ulong spi_open_ms = 0;
	ulong locality_ms = 0;
	ulong startup_ms = 0;
	ulong hashseq_start_ms = 0;
	ulong map_ms = 0;
	ulong update_total_ms = 0;
	ulong update_max_ms = 0;
	ulong update_max_off = 0;
	ulong update_count = 0;
	ulong complete_ms = 0;
	ulong flush_ms = 0;
	ulong release_ms = 0;
	ulong close_ms = 0;
	int rc;

	if (!len || !digest)
		return -EINVAL;

	profile = tpm_tis_spi_env_u32(TPM_TIS_SPI_ENV_PROFILE, 0);
	if (profile) {
		total_start = get_timer(0);
		memset(&g_tpm_tis_spi_update_profile, 0, sizeof(g_tpm_tis_spi_update_profile));
	}

	if (profile)
		stage_start = get_timer(0);
	rc = tpm_tis_spi_open(ctx);
	if (profile)
		spi_open_ms = get_timer(stage_start);
	if (rc) {
		tpm_tis_spi_stage_fail("hash", "spi_open", rc);
		if (profile)
			printf("TPM_TIS_SPI[hash] profile: total=%lu ms spi_open=%lu ms rc=%d\n",
			       get_timer(total_start), spi_open_ms, rc);
		return rc;
	}

	if (profile)
		stage_start = get_timer(0);
	rc = tpm_tis_spi_tis_request_locality(ctx);
	if (profile)
		locality_ms = get_timer(stage_start);
	if (rc) {
		tpm_tis_spi_stage_fail("hash", "request_locality", rc);
		goto out;
	}

	if (profile)
		stage_start = get_timer(0);
	rc = tpm_tis_spi_startup(ctx);
	if (profile)
		startup_ms = get_timer(stage_start);
	if (rc) {
		tpm_tis_spi_stage_fail("hash", "startup", rc);
		goto out_release;
	}

	if (profile)
		stage_start = get_timer(0);
	rc = tpm_tis_spi_hashseq_start_sm3(ctx, &seq);
	if (profile)
		hashseq_start_ms = get_timer(stage_start);
	if (rc) {
		tpm_tis_spi_stage_fail("hash", "hashseq_start", rc);
		goto out_release;
	}

	if (profile)
		stage_start = get_timer(0);
	data = map_sysmem(addr, len);
	if (profile)
		map_ms = get_timer(stage_start);
	if (!data) {
		rc = -ENOMEM;
		tpm_tis_spi_stage_fail("hash", "map_sysmem", rc);
		goto out_release;
	}
	data_cur = data;
	remaining = len;

	chunk = tpm_tis_spi_env_u32(TPM_TIS_SPI_ENV_HASH_CHUNK, TPM_TIS_SPI_HASH_CHUNK_DEFAULT);
	if (chunk == 0 || chunk > TPM_TIS_SPI_HASH_CHUNK_MAX)
		chunk = TPM_TIS_SPI_HASH_CHUNK_DEFAULT;

	if (len >= TPM_TIS_SPI_HASH_PROGRESS_STEP)
		printf("TPM_TIS_SPI[hash] progress: 0x%lx/0x%lx\n", 0UL, len);

	while (remaining) {
		u16 this_len = (u16)((remaining > chunk) ? chunk : remaining);
		ulong update_start = 0;
		ulong update_ms = 0;
		ulong update_off = (ulong)(data_cur - data);

		if (profile)
			update_start = get_timer(0);
		rc = tpm_tis_spi_sequence_update(ctx, seq, data_cur, this_len);
		if (profile) {
			update_ms = get_timer(update_start);
			update_total_ms += update_ms;
			update_count++;
			if (update_ms > update_max_ms) {
				update_max_ms = update_ms;
				update_max_off = update_off;
			}
		}
		if (rc) {
			printf("TPM_TIS_SPI[hash] sequence_update failed off=0x%lx len=0x%x rc=%d\n",
			       update_off, this_len, rc);
			unmap_sysmem(data);
			goto out_flush;
		}
		data_cur += this_len;
		remaining -= this_len;
		processed += this_len;

		if (len >= TPM_TIS_SPI_HASH_PROGRESS_STEP &&
		    (processed >= next_progress || !remaining)) {
			printf("TPM_TIS_SPI[hash] progress: 0x%lx/0x%lx\n",
			       processed, len);
			while (next_progress <= processed)
				next_progress += TPM_TIS_SPI_HASH_PROGRESS_STEP;
		}
	}

	unmap_sysmem(data);
	if (profile)
		stage_start = get_timer(0);
	rc = tpm_tis_spi_sequence_complete(ctx, seq, digest);
	if (profile)
		complete_ms = get_timer(stage_start);
	if (rc) {
		tpm_tis_spi_stage_fail("hash", "sequence_complete", rc);
		goto out_flush;
	}
	seq = 0;
	goto out_release;

out_flush:
	if (seq) {
		int frc;

		if (profile)
			stage_start = get_timer(0);
		frc = tpm_tis_spi_flush_context(ctx, seq);
		if (profile)
			flush_ms += get_timer(stage_start);
		if (frc)
			printf("TPM_TIS_SPI: flush sequence 0x%08x failed: %d\n", seq, frc);
	}
out_release:
	if (profile)
		stage_start = get_timer(0);
	(void)tpm_tis_spi_tis_release_locality(ctx);
	if (profile)
		release_ms = get_timer(stage_start);
out:
	if (profile)
		stage_start = get_timer(0);
	tpm_tis_spi_close(ctx);
	if (profile) {
		close_ms = get_timer(stage_start);
		printf("TPM_TIS_SPI[hash] profile: total=%lu ms rc=%d len=0x%lx chunk=0x%x\n",
		       get_timer(total_start), rc, len, chunk);
		printf("TPM_TIS_SPI[hash] profile: open=%lu locality=%lu startup=%lu hashseq_start=%lu map=%lu complete=%lu flush=%lu release=%lu close=%lu\n",
		       spi_open_ms, locality_ms, startup_ms, hashseq_start_ms,
		       map_ms, complete_ms, flush_ms, release_ms, close_ms);
		printf("TPM_TIS_SPI[hash] profile: update_count=%lu update_total=%lu ms update_avg=%lu ms update_max=%lu ms update_max_off=0x%lx\n",
		       update_count, update_total_ms,
		       update_count ? update_total_ms / update_count : 0,
		       update_max_ms, update_max_off);
		printf("TPM_TIS_SPI[submit] profile: update_count=%lu submit_total=%lu ms submit_avg=%lu ms\n",
		       g_tpm_tis_spi_update_profile.count, g_tpm_tis_spi_update_profile.total_ms,
		       g_tpm_tis_spi_update_profile.count ?
		       g_tpm_tis_spi_update_profile.total_ms / g_tpm_tis_spi_update_profile.count : 0);
		printf("TPM_TIS_SPI[submit] profile: command_ready=%lu write_fifo=%lu sts_go=%lu wait_data_avail=%lu read_rsp_header=%lu read_rsp_body=%lu rsp_parse=%lu\n",
		       g_tpm_tis_spi_update_profile.command_ready_ms,
		       g_tpm_tis_spi_update_profile.write_fifo_ms,
		       g_tpm_tis_spi_update_profile.sts_go_ms,
		       g_tpm_tis_spi_update_profile.wait_data_avail_ms,
		       g_tpm_tis_spi_update_profile.read_rsp_header_ms,
		       g_tpm_tis_spi_update_profile.read_rsp_body_ms,
		       g_tpm_tis_spi_update_profile.rsp_parse_ms);
	}
	return rc;
}

/*
 * 功能：计算内存哈希并与给定十六进制摘要比较。
 */
int tpm_tis_spi_verify_mem_with_hex(ulong addr, ulong len, const char *hex_digest)
{
	u8 expected[TPM_TIS_SPI_DIGEST_SIZE];
	u8 actual[TPM_TIS_SPI_DIGEST_SIZE];
	int rc;

	rc = tpm_tis_spi_parse_digest_hex(hex_digest, expected);
	if (rc)
		return rc;

	rc = tpm_tis_spi_hash_sm3_mem(addr, len, actual);
	if (rc)
		return rc;

	if (memcmp(actual, expected, TPM_TIS_SPI_DIGEST_SIZE)) {
		printf("TPM_TIS_SPI: digest mismatch\n");
		printf("  expected: ");
		tpm_tis_spi_print_digest(expected);
		printf("  actual  : ");
		tpm_tis_spi_print_digest(actual);
		return -EACCES;
	}

	return 0;
}

/*
 * 功能：从环境变量读取摘要并执行内存完整性校验。
 */
int tpm_tis_spi_verify_mem_with_env(ulong addr, ulong len, const char *env_name)
{
	const char *hex;
	const char *name;

	if (!tpm_tis_spi_env_enabled(TPM_TIS_SPI_ENV_VERIFY_ENABLE, 1)) {
		printf("TPM_TIS_SPI: verification disabled by env %s\n",
		       TPM_TIS_SPI_ENV_VERIFY_ENABLE);
		return 0;
	}

	name = env_name ? env_name : TPM_TIS_SPI_ENV_VERIFY_DIGEST;
	hex = getenv(name);
	if (!hex || !hex[0]) {
		printf("TPM_TIS_SPI: missing digest env %s\n", name);
		return -ENOENT;
	}

	return tpm_tis_spi_verify_mem_with_hex(addr, len, hex);
}
