/*
 * debug_stub.c —— X3000 SPL 阶段的串口调试 stub（DDR + 时钟「在线改参数」）
 *
 * 干什么用的（验证工装，不是产品功能）：
 *   上位机（`ds_x3000/ddr_sweep`）在 SPL 阶段下发**参数补丁**（DDR 参数 / PLL 时钟），
 *   让 SPL 拿改后的参数跑原有的 sdram_init()，再做内存自检并把结果回报；
 *   一组参数一次复位，主机据此遍历 PHY 电气档位、系统频率等。
 *
 * 三条设计原则（见设计文档 docs/plans/2026-09-19-debug-stub-ddr-sweep-design.md）：
 *   1. **只改值，不改流程** —— DDR 参数就是改 `g_ddr_param` 指向的结构体；
 *      时钟就是改 `g_clk_values`（pllsetting.c 已改成从它读）；
 *      原有的 pll_init()/clk_init()/sdram_init() 一行都不用动。
 *   2. **脱机不变砖** —— 上电后等主机的窗口有限；等不到就照默认参数正常启动。
 *   3. **不认不跑** —— 参数帧 CRC 不对就 NAK，并且在收到干净参数之前**拒绝 INIT/BOOT**，
 *      绝不带着半截参数去初始化 DDR。
 *
 * 默认**不编译**：要 `#define CONFIG_DDR_DEBUG_STUB`（见 include/configs/x3000_macaw.h）。
 *
 * ── 一、下行帧（主机 -> SPL，WebSocket 二进制帧）
 *      偏移 大小 字段
 *      0    2    MAGIC   0x44 0x53（'D' 'S'）
 *      2    1    CMD
 *      3    1    SEQ     主机填；回报原样带回（配对请求/回应）
 *      4    2    LEN     payload 字节数（小端）
 *      6    2    CRC16   覆盖 **CMD,SEQ,LEN,PAYLOAD**（不含 MAGIC 与 CRC 本身），
 *                        CRC16-CCITT：多项式 0x1021，初值 0xFFFF，不反转
 *      8    N    PAYLOAD
 *   ⚠️ 必须用 **WebSocket** 发：控制板的 HTTP `/serial/send` 走 strlen + JSON 转义，
 *      发不了含 0x00 的二进制（细节见设计文档 §3.3）。
 *
 * ── 二、上行（SPL -> 主机，**全可打印文本行，一律 \n 结尾**）
 *      首 token  例子
 *      READY     `READY 1 tag=3756/4b0d words=278/17`
 *      PONG      `PONG 3 ver=1`
 *      PARAM     `PARAM 4 n=8`
 *      SETALL    `SETALL 5 n=278`
 *      INIT      `INIT 6 rc=0 cpcsr=80000000`
 *      MEM       `MEM 7 ok n=1024` / `MEM 7 bad addr=81000004 got=0 want=5a5a5a5a`
 *      DUMP      `DUMP 8 end`
 *      PEEK      `PEEK 9 addr=13012000 val=1`
 *      BOOT      `BOOT 10`
 *      ERR/NAK   `NAK 11 crc` / `ERR 11 cmd`
 *   （走文本行是因为控制板的日志缓冲只推 \n 结尾的行，还会带时间戳。）
 *
 * ── 三、命令
 *      0x01 PING    -                                     -> PONG
 *      0x02 PARAM   u8 dom + n*(u16 word, u32 val)        -> PARAM
 *      0x03 SETALL  u8 dom + 整块（域大小字节，小端字序） -> SETALL
 *      0x04 INIT    u8 flags（bit0=再跑一次默认自检, bit1=不继续启动） -> INIT
 *      0x05 MEMTEST u8 mode + u32 addr + u32 len + u32 pat -> MEM
 *      0x06 DUMP    u8 region（0=全部 1=CPM 2=DDRP 3=DDRC） -> DUMP
 *      0x07 PEEK    u32 addr                              -> PEEK
 *      0x08 POKE    u32 addr + u32 val                    -> PEEK
 *      0x09 BOOT    -                                     -> BOOT（继续原启动流程）
 *
 * ── 四、RAM 占用（SRAM 只有二十几 KB，所以刻意省）
 *      SETALL **边收边写进结构体**，不占整块缓冲；只有小命令用 `s_rx[STUB_RX_MAX]`。
 */
#include <config.h>
#include <common.h>
#include <serial.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>

#include "ddr.h"			/* g_ddr_param / sdram_init() */
#include "debug_stub_param_layout.h"	/* 自动生成：两域的字数/大小/指纹 */
#include "../x3000_clk_values.h"	/* g_clk_values（时钟域） */

/*
 * ddr.c 里的 dump 没写进 ddr.h。为两行原型去动公共头不值当，先自己声明
 * （符号就在同一个 SPL 镜像里）。
 */
extern void ddr_dump_region(int which);
extern void ddr_dump_all(void);

#define STUB_VER	1

/* 帧 */
#define M0		0x44		/* 'D' */
#define M1		0x53		/* 'S' */
#define HDR_LEN		8
#define STUB_RX_MAX	320		/* 小命令 payload 上限（够 PARAM/DUMP/PEEK 用）*/
#define STUB_MAX_PATCH	48		/* 一帧最多几个补丁（48*6=288 <= RX_MAX）*/

/* 命令 */
#define CMD_PING	0x01
#define CMD_PARAM	0x02
#define CMD_SETALL	0x03
#define CMD_INIT	0x04
#define CMD_MEMTEST	0x05
#define CMD_DUMP	0x06
#define CMD_PEEK	0x07
#define CMD_POKE	0x08
#define CMD_BOOT	0x09

/* 参数域 */
#define DOM_DDR		0
#define DOM_CLK		1

/* INIT flags */
#define INIT_F_SELFTEST	0x01		/* init 后顺手跑一次默认窗口自检 */
#define INIT_F_KEEP	0x02		/* 别往下启动（回报后继续听命令）*/

/*
 * ⭐⭐ **两个阶段**（用户 2026-09-21 指出，这是原来的设计缺陷）：
 *
 *   阶段 1 —— `DS_PHASE_CLK`（`soc.c` 里 `pll_init()` **之前**，UART 刚可用）：
 *     **只协商参数**：收 PARAM/SETALL 改 PLL / 分频 / DDR 参数；
 *     ⛔ **不能**跑 DDR 初始化或自检 —— 此刻 `pll_init()`/`clk_init()` 还没跑
 *        （它们在本函数 return 之后才执行），DDR 时钟根本没起来。
 *        旧代码在这里调 `sdram_init()` 必然 `wait clk 0 timeout`（真板实测）。
 *     参数改完后 `return`，让**原始流程**去 `pll_init()` → `clk_init()` →
 *     `sdram_init()` —— 参数就是这样生效的。
 *
 *   阶段 2 —— `DS_PHASE_DDR`（`sdram_init()` **之后**）：
 *     DDR 已经能用了 ⇒ 进来先打 `READY2`（**"没卡在 DDR 初始化"的证据**），
 *     然后上位机在这里做 DDR 读写测试（MEMTEST）拿初步结论。
 *     参数只能在阶段 1 改：阶段 2 收到 SETALL/PARAM/INIT 一律拒
 *     （改了就与"已经初始化好的 DDR"不一致，必须重新复位）。
 *
 *   ⇒ 上位机的判定链（用户要的"参数是否合适"的评判标准）：
 *       ① 阶段 1 参数确认（ACK）
 *       ② 阶段 2 的 READY2 有没有来（**卡在 DDR 初始化 = 这组参数不行**）
 *       ③ 阶段 2 的读写测试 pass/fail
 *       ④ U-Boot 启动完成 / 超时
 *       ⑤ kernel 启动完成 / 超时
 */
#define DS_PHASE_CLK	1
#define DS_PHASE_DDR	2

/* MEMTEST mode */
#define MEM_MODE_NONE	0		/* 只写 */
#define MEM_MODE_RW	1		/* 写后读回比较 */
#define MEM_MODE_ADDR	2		/* 地址走位（写 (addr^pattern) 再读回）*/

/*
 * 默认自检窗口：DDR 里 u-boot proper 装载地址（物理 0x00100000）之后 16MB 处的
 * 一小段。挑这里是因为此刻 SPL 还没把 u-boot 搬过来，这一段一定没被用；真实遍历
 * 时上位机会用 MEMTEST 明确指定地址/长度，不依赖这个默认值。
 *
 * ⚠️⚠️ **必须是 KSEG1（uncached，0xa0000000 起），不能用 KSEG0（cached）**：
 *   KSEG0（0x80000000~0x9fffffff）是 **cached** 的 —— 写进去读回来可能**全程在
 *   cache 里完成、根本到不了 DRAM**，"写进去读回来"永远相等 ⇒ **坏参数假通过**。
 *   对遍历工具这是致命的（会把不通的参数判成好参数）。KSEG1 每次访问都直达 DRAM
 *   控制器，测的才是真 DDR。
 *   0xa1000000 = KSEG1 视图，物理仍是 0x01000000（与原来的 0x81000000 同一物理址）。
 *   （MIPS 地址窗口：KSEG1 是 KSEG0 的 uncached 别名，物理偏移相同。）
 */
#define SELFTEST_ADDR	0xa1000000u
#define SELFTEST_LEN	0x1000u

/*
 * 等主机的窗口（**时间**，用 `get_ticks()` = OST 计数器判定）。
 *   未连过主机：**500ms** —— 等不到就照默认参数正常启动（脱机不变砖）。
 *   已连过主机：**5s** —— 上位机发多帧时帧间有间隔，窗口要留够（用户要求 5s 保险）。
 *
 * ⚠️ 上位机的**约定**：复位之后要**立即盲发**命令（不用先等 READY）。
 *    因为 500ms 这个窗口不长，而"等 READY 再发"要多绕一圈 HTTP 轮询，
 *    很容易错过窗口。帧格式本身能重同步（MAGIC 状态机 + CRC），
 *    所以盲发是安全的；READY 之后从控制板日志缓冲里照样读得到（它不会丢）。
 *    窗口值可用 -D 覆盖（主机侧模拟器按时间尺度缩小用）。
 *
 * ⚠️ 代价与对策：主机必须在这 0.5s 内把第一帧发出来。上位机从 `power_cycle` 返回
 *    到"真正发出第一个字节"要过 控制板 HTTP + WebSocket 建连 + 串口 ——
 *    **参考值**：2026-09-21 那次联机成功时，主机用了 **0.85s** 才连上
 *    （`power_cycle` → `[ds] host up`）。⇒ 若出现"发了但板子没收到"
 *    （NO_READY / 假连上），把 `DS_IDLE_MS_INIT` 加大，
 *    或把主机侧的发帧时机提前。
 *
 * ─── 为什么从"循环计数"改成"时间"（用户 2026-09-21 的要求）──────────────
 * 原来这两个窗口是 `IDLE_LOOPS_*`（数循环）。用户原话："你用的是 loops 来做检测，
 * 这个是用来检测字符多少次没有字符的一个变量，跟时间关系有、但是不精确。你可以用
 * get_ticks 来做超时判断，这样更精确。"
 *
 * ⚠️ 确实不精确 —— 实测同一份循环，两段差 **20 倍**（2026-09-21）：
 *   · **阶段 1**（`pll_init()` 之前，CPU 在低速时钟上）：`0x300000`(3,145,728 次)
 *     = **11.8s** ⇒ **≈3.75µs/次**；
 *   · **阶段 2**（PLL 已配好、CPU 高速）：`0x8000000`(134M 次) = **24.15s**
 *     ⇒ **≈180ns/次**。
 *   我按阶段 2 的数去推阶段 1，算出"0.55s"、实测 11.8s —— 同一个坑踩了两次。
 *   根因：每次循环的耗时随 **CPU 频率（PLL 前/后）**、**编译优化**、
 *   **读 UART 寄存器的开销**变，**本质不精确**。
 *
 * ⚠️ 阶段 2 也**不用怕 PLL 改过**（这正是当初不敢用 timer 的理由）：阶段 2 的窗口
 *    量的是**相对**时长，不是绝对时间；`get_ticks()` 是 OST 的单调计数器，
 *    PLL 变了只影响"1ms 是多少 tick"的换算 —— 而这里用的 `multiple` 与
 *    `timer.c` 的 `__udelay()` **同一个系数**，两者始终自洽。
 */
#ifndef DS_IDLE_MS_INIT
#define DS_IDLE_MS_INIT		1000u	/* 没连过主机：500ms 内没字符就走 */
#endif
#ifndef DS_IDLE_MS_CONN
#define DS_IDLE_MS_CONN		5000u	/* 连过主机：等下一帧（帧间有间隔）*/
#endif
/*
 * 阶段 2（`sdram_init()` 之后）等主机的窗口。
 * ⭐ 用户 2026-09-21："阶段 2 应该是可选的做一下 ddr 读写测试，然后**直接跳出**
 *    进正常启动模式。" ⇒ 自检（可选）跑完给一个窗口，让主机来得及发 MEMTEST/BOOT；
 *    等不到就正常收尾回到启动流程 —— 自检结果早已用 `MEM 0 …` 报出去了。
 * ⚠️ 用户 2026-09-21 明确了**脱机/联机的区别**："阶段 2，是在脱机时直接返回，
 *    联机的时候，还是走 dump 自检，memtest boot 这个不变化。"
 *    ⇒ 脱机（阶段 1 没等到主机）在 `debug_stub_run()` 开头就 return 了（什么都不做，
 *      连 CPM dump 都不打）；**联机**才走 dump + 自检 + 等 MEMTEST/BOOT ——
 *      这里就是那个"等 MEMTEST/BOOT"的窗口。
 */
#ifndef DS_IDLE_MS_PHASE2
#define DS_IDLE_MS_PHASE2	DS_IDLE_MS_CONN
#endif

/*
 * 时间换算 —— `get_ticks()` 返回的是 **OST 原始计数**，不是 µs/ms：
 *   `timer.c`:  multiple = CONFIG_SYS_EXTAL / 1000000 / G_OST_DIV  ⇒ **1µs 的 tick 数**
 *               `__udelay()` 用的就是 `usec * multiple` ⇒ 这里的换算与它**严格一致**。
 * ⚠️ `multiple` 是在 `timer_init()` 里赋值的 ⇒ `soc.c` 必须把 `debug_stub_run()`
 *    放在 `timer_init()` **之后**（用户 2026-09-21 的设计）。
 */
extern unsigned int multiple;

/* 从现在起 ms 毫秒之后的 deadline（绝对 `get_ticks()` 值）*/
static unsigned long long ds_deadline(unsigned int ms)
{
	return get_ticks() + (unsigned long long)ms * 1000ull * multiple;
}

/* 从 start 到现在过了多少 ms（**只在握手/收尾时调**，除法不进热路径）*/
static unsigned int ds_elapsed_ms(unsigned long long start)
{
	unsigned int per_ms = multiple * 1000u;

	return per_ms ? (unsigned int)((get_ticks() - start) / per_ms) : 0;
}

static unsigned char s_rx[STUB_RX_MAX];
static int s_param_ok;			/* 收过 CRC 干净的参数帧（否则不许 INIT/BOOT）*/
static unsigned int s_mt_addr;		/* 自检失败细节（0 = 没失败）*/
static unsigned int s_mt_got;
static unsigned int s_mt_want;
static unsigned int s_got;		/* 本轮 read_frame 一共收到几个字节 */
static int s_want_selftest;		/* 阶段 1 的 INIT 带 SELFTEST -> 阶段 2 自动跑 */
/*
 * ⚠️ 下面这些都是 **static（BSS）**，而 `debug_stub_run()` 跑在 `soc.c` 清 BSS
 *    **之前** ⇒ 不能指望"上电就是 0"，**必须由阶段 1 入口显式清零**
 *    （见 `debug_stub_run()` 开头；用户 2026-09-22 真板抓到 `s_host_seen`
 *    非 0 导致阶段 2 无条件进入）。
 */
static int s_host_seen;			/* 阶段 1 见过主机 -> 阶段 2 才值得等它 */

/*
 * 板端调试输出（DS_DEBUG）—— 用户 2026-09-21："在 debug_stub 端应该也要加入
 * 一些 debug 才好，现在不知道什么情况了。"
 *
 *   0 = 关（只保留协议回执；SPL 体积紧张时用 `-DDS_DEBUG=0` 编）
 *   1 = **默认**：连接建立 / 每帧摘要 / 校验失败细节 / **半截帧收到多少字节** /
 *       INIT 前后 / 超时时等了多少 —— 排查"通信不通"就看这些
 *
 * ⚠️ SPL 的 printf 是精简版（没有 %c、没有宽度修饰），这里一律用
 *    o_str/o_dec/o_hex 手工拼；**不引 printf/vsnprintf**（体积 + SPL 里未必可用）。
 * ⚠️ 上位机（`ddr_sweep/runner.py`）会把板端输出原样抄进实时日志的
 *    「板端输出尾部」，所以每行只讲一件事、要短。
 */
#ifndef DS_DEBUG
#define DS_DEBUG 1
#endif

/* ============================ 输出 ============================ */
/* SPL 的 printf 是精简版（没有 %c、没有宽度修饰），字符/定长十六进制自己来 */
static void o_ch(char c)
{
	serial_putc(c);
}

static void o_str(const char *s)
{
	while (*s)
		o_ch(*s++);
}

static void o_nl(void)
{
	o_ch('\n');
}

static void o_hex(unsigned int v, int digits)
{
	static const char d[] = "0123456789abcdef";

	while (digits--)
		o_ch(d[(v >> (digits * 4)) & 0xf]);
}

static void o_dec(unsigned int v)
{
	char b[11];
	int i = 0;

	if (!v) {
		o_ch('0');
		return;
	}
	while (v) {
		b[i++] = (char)('0' + (v % 10u));
		v /= 10u;
	}
	while (i--)
		o_ch(b[i]);
}

/* ============================ 板端调试 ============================ */
#if DS_DEBUG
/* 收到一帧（帧头解析出来就打；CRC 结论由 NAK/PARAM 那些行给出）*/
static void d_rx(unsigned char cmd, unsigned char seq, unsigned int len,
		 unsigned short crc)
{
	o_str("[ds] rx cmd=");
	o_hex(cmd, 2);
	o_str(" seq=");
	o_dec(seq);
	o_str(" len=");
	o_dec(len);
	o_str(" crc=");
	o_hex(crc, 4);
	o_nl();
}

/*
 * 一帧没读全就断了 —— **"板子像进了接收但不回话"的头号原因**：
 * 要么半截帧（线/波特率/流控），要么主机发一半自己断了。
 * `got/expect`（这一帧收到多少 / 应该多少）一看就知道。
 */
static void d_drop(const char *where, unsigned int got, unsigned int expect)
{
	o_str("[ds] drop ");
	o_str(where);
	o_str(" got=");
	o_dec(got);
	o_ch('/');
	if (expect)
		o_dec(expect);
	else
		o_ch('?');
	o_str(" total=");
	o_dec(s_got);
	o_nl();
}

/* `[ds] key=值`（digits 非 0 用十六进制定长，否则十进制）*/
static void d_kv(const char *k, unsigned int v, int digits)
{
	o_str("[ds] ");
	o_str(k);
	o_str("=");
	if (digits)
		o_hex(v, digits);
	else
		o_dec(v);
	o_nl();
}
#else
#define d_rx(a, b, c, d)	do { (void)(a); (void)(b); (void)(c); (void)(d); } while (0)
#define d_drop(a, b, c)		do { (void)(a); (void)(b); (void)(c); } while (0)
#define d_kv(a, b, c)		do { (void)(a); (void)(b); (void)(c); } while (0)
#endif

/* ============================ 小工具 ============================ */
static unsigned short crc16(unsigned short crc, unsigned char b)
{
	int i;

	crc ^= (unsigned short)b << 8;
	for (i = 0; i < 8; i++)
		crc = (crc & 0x8000) ? (unsigned short)((crc << 1) ^ 0x1021)
				     : (unsigned short)(crc << 1);
	return crc;
}

/*
 * 超时读一个字节；到 deadline 还没数据就返回 -1。
 * ⚠️ `deadline` 是**绝对**的 `get_ticks()` 值，由 `read_frame()` 在入口算一次 ——
 *    所以"一帧的总预算"是固定的（半截帧也不会无限等下去）。
 *    单位见上面 `ds_deadline()` 的注释（`get_ticks()` 是 OST 原始计数）。
 */
static int rx_byte(unsigned long long deadline)
{
	int c;

	while (!serial_tstc()) {
		if (get_ticks() >= deadline)
			return -1;
	}
	c = serial_getc() & 0xff;
	s_got++;			/* 半截帧排查要用：这一帧一共收到几个字节 */
	return c;
}

/* ============================ 参数域 ============================ */
static void *dom_base(int dom, int *words)
{
	switch (dom) {
	case DOM_DDR:
		*words = DDR_PARAM_WORDS;
		return (void *)g_ddr_param;	/* ddr.c 的 g_ddr_param 指向它 */
	case DOM_CLK:
		*words = CLK_PARAM_WORDS;
		return (void *)&g_clk_values;
	default:
		*words = 0;
		return 0;
	}
}

static unsigned int dom_size(int dom)
{
	return (dom == DOM_CLK) ? CLK_PARAM_SIZE : DDR_PARAM_SIZE;
}

/* 按**字索引**改一个字段；越界即报错 —— 这是"只动自己的参数块"的安全边界 */
static int dom_poke_word(int dom, unsigned int word, unsigned int val)
{
	int words;
	void *base = dom_base(dom, &words);

	if (!base || word >= (unsigned int)words)
		return -1;
	((unsigned int *)base)[word] = val;
	return 0;
}

/* ============================ 内存自检 ============================ */
/* 返回 0 = 全部通过；否则失败细节在 s_mt_*（s_mt_addr 非 0）*/
static int memtest_run(unsigned char mode, unsigned int addr,
		       unsigned int len, unsigned int pattern)
{
	unsigned int i, v, want;

	s_mt_addr = s_mt_got = s_mt_want = 0;
	len &= ~3u;				/* 按 32 位对齐 */
	if (!len)
		return 0;

	for (i = 0; i < len; i += 4) {
		want = (mode == MEM_MODE_ADDR) ? ((addr + i) ^ pattern)
					       : (pattern ^ i);
		*(volatile unsigned int *)(addr + i) = want;
	}
	if (mode == MEM_MODE_NONE)
		return 0;

	for (i = 0; i < len; i += 4) {
		want = (mode == MEM_MODE_ADDR) ? ((addr + i) ^ pattern)
					       : (pattern ^ i);
		v = *(volatile unsigned int *)(addr + i);
		if (v != want) {
			s_mt_addr = addr + i;
			s_mt_got = v;
			s_mt_want = want;
			return -1;
		}
	}
	return 0;
}

static void memtest_reply(unsigned char seq, unsigned int len)
{
	o_str("MEM ");
	o_dec(seq);
	o_ch(' ');
	if (!s_mt_addr) {
		o_str("ok n=");
		o_dec(len & ~3u);
		o_nl();
		return;
	}
	o_str("bad addr=");
	o_hex(s_mt_addr, 8);
	o_str(" got=");
	o_hex(s_mt_got, 8);
	o_str(" want=");
	o_hex(s_mt_want, 8);
	o_nl();
}

/* ============================ 一帧的处理 ============================ */
/* 返回 1 = 继续正常启动；0 = 留在 stub */
static int handle_frame(const unsigned char *h, unsigned int len, int phase)
{
	unsigned char cmd = h[2];
	unsigned char seq = h[3];
	unsigned int i;

	/*
	 * 阶段 2 是**只读**的：DDR 已经按阶段 1 的参数初始化好了，
	 * 再改参数就与"已经初始化好的 DDR"不一致 —— 拒掉并说清怎么办。
	 */
	if (phase == DS_PHASE_DDR &&
	    (cmd == CMD_SETALL || cmd == CMD_PARAM || cmd == CMD_INIT)) {
		o_str("ERR ");
		o_dec(seq);
		o_str(" phase2-ro\n");
		o_str("[ds] 阶段2 只做测试：参数请在阶段1（pll_init 之前）改，"
		      "改完要重新复位\n");
		return 0;
	}

	switch (cmd) {
	case CMD_PING:
		o_str("PONG ");
		o_dec(seq);
		o_str(" ver=");
		o_dec(STUB_VER);
		o_nl();
		break;

	case CMD_PARAM: {
		unsigned int dom = s_rx[0];
		unsigned int n = s_rx[1];
		int bad = 0;

		if (n > STUB_MAX_PATCH || len < 2 + n * 6)
			bad = 1;
		for (i = 0; !bad && i < n; i++) {
			const unsigned char *p = &s_rx[2 + i * 6];
			unsigned int word = (unsigned int)(p[0] | (p[1] << 8));
			unsigned int val = (unsigned int)(p[2] | (p[3] << 8) |
							  (p[4] << 16) | (p[5] << 24));

			if (dom_poke_word((int)dom, word, val))
				bad = 1;
		}
		if (bad) {
			o_str("ERR ");
			o_dec(seq);
			o_str(" param-range\n");
		} else {
			s_param_ok = 1;
			if (n) {
				const unsigned char *p = &s_rx[2];

				o_str("[ds] param dom=");
				o_dec(dom);
				o_str(" n=");
				o_dec(n);
				o_str(" first#=");
				o_dec((unsigned int)(p[0] | (p[1] << 8)));
				o_str(" =");
				o_hex((unsigned int)(p[2] | (p[3] << 8) |
						     (p[4] << 16) | (p[5] << 24)), 8);
				o_nl();
			}
			o_str("PARAM ");
			o_dec(seq);
			o_str(" n=");
			o_dec(n);
			o_nl();
		}
		break;
	}

	case CMD_SETALL: {
		int words = 0;
		void *base = dom_base((int)s_rx[0], &words);

		/* 数据已经在 read_frame 里边收边写进去了，这里只回报（LEN 含域字节）*/
		if (!base || len != dom_size((int)s_rx[0]) + 1) {
			o_str("ERR ");
			o_dec(seq);
			o_str(" setall-len\n");
			break;
		}
		s_param_ok = 1;
		/* 写到哪 / 多少字 / 多大 / 首字 —— 复盘"这一组把哪一块改成了什么" */
		o_str("[ds] wrote dom=");
		o_dec((unsigned int)s_rx[0]);
		o_str(" words=");
		o_dec((unsigned int)words);
		o_str(" size=");
		o_dec(dom_size((int)s_rx[0]));
		o_str(" first=");
		o_hex(((unsigned int *)base)[0], 8);
		o_nl();
		o_str("SETALL ");
		o_dec(seq);
		o_str(" n=");
		o_dec((unsigned int)words);
		o_nl();
		break;
	}

	case CMD_INIT: {
		unsigned char flags = s_rx[0];

		/* 进 INIT 前把"手上有什么参数"讲清楚：no-param 是最常见的一种失败 */
		o_str("[ds] init flags=");
		o_hex(flags, 2);
		o_str(" param_ok=");
		o_dec((unsigned int)s_param_ok);
		o_str(" cpcsr=");
		o_hex(cpm_inl(CPM_CPCSR), 8);
		o_nl();
		if (!s_param_ok) {
			/* 没收到干净参数前不许初始化，免得带着半截参数把板子带偏 */
			o_str("ERR ");
			o_dec(seq);
			o_str(" no-param\n");
			break;
		}

		/*
		 * ⛔ **这里绝不调 `sdram_init()`** —— 旧代码就是在这儿调的，而本函数
		 *    在 `pll_init()`/`clk_init()` **之前**运行（见 soc.c），DDR 时钟
		 *    根本没起来 ⇒ 必然 `wait clk 0 timeout`（真板实测，用户指出）。
		 *    参数此刻已经写进 `g_ddr_param` / `g_clk_values`：
		 *    接下来 return，让**原始流程**用新值去跑
		 *    `pll_init()` → `clk_init()` → `sdram_init()` —— 那才是参数生效点。
		 */
		o_str("ACK ");
		o_dec(seq);
		if (flags & INIT_F_SELFTEST)
			o_str(" selftest=phase2");
		o_nl();
		if (flags & INIT_F_SELFTEST)
			s_want_selftest = 1;	/* 记下来，阶段 2 自动跑一次 */
		if (!(flags & INIT_F_KEEP)) {
			o_str("[ds] 参数已收到 -> 交给原始流程 "
			      "pll_init/clk_init/sdram_init\n");
			return 1;	/* 继续正常启动（参数从这里生效）*/
		}
		break;			/* KEEP：留在阶段 1 等更多命令 */
	}

	case CMD_MEMTEST: {
		unsigned char mode = s_rx[0];
		unsigned int addr = (unsigned int)(s_rx[1] | (s_rx[2] << 8) |
						   (s_rx[3] << 16) | (s_rx[4] << 24));
		unsigned int mlen = (unsigned int)(s_rx[5] | (s_rx[6] << 8) |
						   (s_rx[7] << 16) | (s_rx[8] << 24));
		unsigned int pat = (unsigned int)(s_rx[9] | (s_rx[10] << 8) |
						  (s_rx[11] << 16) | (s_rx[12] << 24));

		if (len < 13) {
			o_str("ERR ");
			o_dec(seq);
			o_str(" memtest-len\n");
			break;
		}
		o_str("[ds] memtest mode=");
		o_dec(mode);
		o_str(" addr=");
		o_hex(addr, 8);
		o_str(" len=");
		o_dec(mlen);
		o_str(" pat=");
		o_hex(pat, 8);
		o_nl();
		(void)memtest_run(mode, addr, mlen, pat);
		memtest_reply(seq, mlen);
		break;
	}

	case CMD_DUMP:
		switch (s_rx[0]) {
		case 1:
			ddr_dump_region(0);
			break;
		case 2:
			ddr_dump_region(1);
			break;
		case 3:
			ddr_dump_region(2);
			break;
		default:
			ddr_dump_all();
			break;
		}
		o_str("DUMP ");
		o_dec(seq);
		o_str(" end\n");
		break;

	case CMD_PEEK:
	case CMD_POKE: {
		unsigned int addr = (unsigned int)(s_rx[0] | (s_rx[1] << 8) |
						   (s_rx[2] << 16) | (s_rx[3] << 24));

		if (addr & 3) {
			o_str("ERR ");
			o_dec(seq);
			o_str(" align\n");
			break;
		}
		if (cmd == CMD_POKE) {
			unsigned int val = (unsigned int)(s_rx[4] | (s_rx[5] << 8) |
							  (s_rx[6] << 16) | (s_rx[7] << 24));
			*(volatile unsigned int *)addr = val;
		}
		o_str("PEEK ");
		o_dec(seq);
		o_str(" addr=");
		o_hex(addr, 8);
		o_str(" val=");
		o_hex(*(volatile unsigned int *)addr, 8);
		o_nl();
		break;
	}

	case CMD_BOOT:
		o_str("BOOT ");
		o_dec(seq);
		o_nl();
		return 1;

	default:
		o_str("ERR ");
		o_dec(seq);
		o_str(" cmd\n");
		break;
	}

	return 0;
}

/* ============================ 收一帧 ============================ */
static unsigned short hdr_crc(const unsigned char *h)
{
	unsigned short c = 0xffff;

	c = crc16(c, h[2]);
	c = crc16(c, h[3]);
	c = crc16(c, h[4]);
	c = crc16(c, h[5]);
	return c;
}

/*
 * 返回 1 = 收到一帧（处理完，*boot 说明要不要继续启动）；0 = 超时。
 * MAGIC 用状态机找，半截帧/噪声不会卡住状态机。
 * ⚠️ `deadline`（绝对 `get_ticks()` 值）在这一帧内**固定**：找 MAGIC + 收帧体
 *    共用同一个预算（与原来 `loops` 递减的语义一致，只是换成时间）。
 */
static int read_frame(unsigned long long deadline, int *boot, int phase)
{
	unsigned char h[HDR_LEN];
	unsigned int i, len;
	unsigned short crc, calc;
	int c, state;

	/*
	 * ⚠️ 每次都先清 *boot：NAK/超时等路径上不会写它，而调用方立刻会读
	 * —— 不清就是读栈上垃圾，会**莫名其妙地提前跳去正常启动**
	 * （模拟器实测抓到的 bug，上板会更难查）。
	 */
	*boot = 0;
	s_got = 0;

	/* 找 MAGIC 0x44 0x53 */
	state = 0;
	for (;;) {
		c = rx_byte(deadline);
		if (c < 0) {
			/* 整个窗口里一个字节都没等到 —— 没主机（脱机）/串口不通 */
			d_drop("no-byte", s_got, 0);
			return 0;
		}
		if (c == M0) {
			state = 1;
			continue;
		}
		if (state == 1 && c == M1)
			break;
		state = 0;
	}
	h[0] = M0;
	h[1] = M1;

	for (i = 2; i < HDR_LEN; i++) {
		c = rx_byte(deadline);
		if (c < 0) {
			d_drop("hdr", i, HDR_LEN);
			return 0;
		}
		h[i] = (unsigned char)c;
	}
	len = (unsigned int)(h[4] | (h[5] << 8));
	crc = (unsigned short)(h[6] | (h[7] << 8));
	calc = hdr_crc(h);
	d_rx(h[2], h[3], len, crc);

	if (h[2] == CMD_SETALL && len >= 1) {
		/*
		 * 整块：**边收边写**（不占整块缓冲）。域字节先到，用来定位基址；
		 * 长度必须与域大小严格一致，否则把余下字节吃掉再报错（别污染下一帧）。
		 */
		int words = 0;
		void *base;
		unsigned int k, val;

		/*
		 * ⚠️ 阶段 2 **只读**：整块参数不许再写 —— 写了就与"已经按阶段 1 参数
		 * 初始化好的 DDR"不一致（`handle_frame` 那边的拒绝太晚了：这里"边收边写"
		 * 已经把数据落进结构体了）。所以必须**先**把整块读完丢掉（保持流对齐，
		 * 否则余下字节会被当成下一帧），再回 ERR。
		 */
		if (phase == DS_PHASE_DDR) {
			for (i = 0; i < len; i++) {
				c = rx_byte(deadline);
				if (c < 0) {
					d_drop("setall-p2", i, len);
					return 0;
				}
			}
			o_str("ERR ");
			o_dec(h[3]);
			o_str(" phase2-ro\n");
			return 1;
		}

		c = rx_byte(deadline);
		if (c < 0) {
			d_drop("setall-dom", s_got, len + HDR_LEN);
			return 0;
		}
		s_rx[0] = (unsigned char)c;
		calc = crc16(calc, (unsigned char)c);
		base = dom_base((int)s_rx[0], &words);

		/* ⚠️ LEN 含前面的域字节，所以是 dom_size() + 1 */
		if (!base || len != dom_size((int)s_rx[0]) + 1) {
			c = rx_byte(deadline);
			if (c < 0)
				return 0;
			calc = crc16(calc, (unsigned char)c);
			for (i = 2; i < len; i++) {
				c = rx_byte(deadline);
				if (c < 0) {
					d_drop("setall-flush", i, len);
					return 0;
				}
				calc = crc16(calc, (unsigned char)c);
			}
			o_str("ERR ");
			o_dec(h[3]);
			o_str(" setall-len\n");
			/* 长度对不上时把"我们要多少"讲清楚：两边头文件不同版
			   （u-boot 的 sizeof(struct ddr_param) 与主机字段表不一致）*/
			o_str("[ds] setall-len dom=");
			o_dec((unsigned int)s_rx[0]);
			o_str(" want=");
			o_dec(dom_size((int)s_rx[0]) + 1);
			o_nl();
			return 1;
		}

		k = 0;
		val = 0;
		for (i = 1; i < len; i++) {
			c = rx_byte(deadline);
			if (c < 0) {
				/* ⭐ 最常见的一条：数据没收完（线/波特率/流控/主机发一半断了）。
				   `got/expect` 一看就知道差多少 —— 上板排查第一眼就看它。 */
				d_drop("setall-data", i, len);
				return 0;
			}
			calc = crc16(calc, (unsigned char)c);
			val |= ((unsigned int)c) << (8 * (k & 3));
			if ((k & 3) == 3) {
				((unsigned int *)base)[k >> 2] = val;
				val = 0;
			}
			k++;
		}
		if (calc != crc) {
			s_param_ok = 0;	/* 半截参数：禁止后续 INIT/BOOT */
			o_str("NAK ");
			o_dec(h[3]);
			o_str(" crc\n");
			o_str("[ds] setall crc calc=");
			o_hex(calc, 4);
			o_str(" want=");
			o_hex(crc, 4);
			o_nl();
			return 1;
		}
		d_kv("setall ok dom", (unsigned int)s_rx[0], 0);
		*boot = handle_frame(h, len, phase);
		return 1;
	}

	if (len > STUB_RX_MAX) {
		/* 太长：读完丢掉（保持流对齐），然后按 CRC 失配处理 */
		unsigned int extra = len - STUB_RX_MAX;

		for (i = 0; i < STUB_RX_MAX; i++) {
			c = rx_byte(deadline);
			if (c < 0) {
				d_drop("long-frame", s_got, len + HDR_LEN);
				return 0;
			}
			s_rx[i] = (unsigned char)c;
		}
		while (extra--) {
			c = rx_byte(deadline);
			if (c < 0) {
				d_drop("long-frame-flush", s_got, len + HDR_LEN);
				return 0;
			}
		}
		o_str("NAK ");
		o_dec(h[3]);
		o_str(" len\n");
		d_kv("frame too long", len, 0);
		return 1;
	}

	for (i = 0; i < len; i++) {
		c = rx_byte(deadline);
		if (c < 0) {
			d_drop("data", i, len);
			return 0;
		}
		s_rx[i] = (unsigned char)c;
		calc = crc16(calc, s_rx[i]);
	}
	if (calc != crc) {
		o_str("NAK ");
		o_dec(h[3]);
		o_str(" crc\n");
		o_str("[ds] crc calc=");
		o_hex(calc, 4);
		o_str(" want=");
		o_hex(crc, 4);
		o_str(" got=");
		o_dec(s_got);
		o_str("/");
		o_dec(len + HDR_LEN);
		o_nl();
		return 1;			/* CRC 错不算超时，主机可以重发 */
	}

	*boot = handle_frame(h, len, phase);
	return 1;
}

/* ============================ 入口 ============================ */
/*
 * `phase` 见 `DS_PHASE_*`：两阶段共用帧解析与参数块，差别只在"能干什么"。
 *   阶段 1（`pll_init()` 之前）：协商参数；`INIT` = 参数确认后交还原始流程；
 *   阶段 2（`sdram_init()` 之后）：**只读** —— DDR 读写测试/DUMP/PEEK/POKE。
 */
void debug_stub_run(int phase)
{
	unsigned long long deadline, t0;
	unsigned int connected = 0;
	unsigned int used_ms;
	int boot, got;
	int p2 = (phase == DS_PHASE_DDR);

	/*
	 * ⚠️ `multiple` = 1µs 的 OST tick 数（`timer.c` 在 `timer_init()` 里算），
	 *    本文件的超时判定全靠它 ⇒ 为 0 会让 deadline **立刻到期**（表现为"主机
	 *    永远连不上"，很难查）。`soc.c` 已经把 `timer_init()` 提到本函数之前了，
	 *    这里只是防呆：真为 0 就按 EXTAL/8 分频（24MHz→3MHz）兜一个并报警。
	 */
	if (!multiple) {
		multiple = CONFIG_SYS_EXTAL / 1000000u / 8u;
		if (!multiple)
			multiple = 1;
		o_str("[ds] warn: timer not init, multiple forced\n");
	}


	/*
	 * ⭐ **脱机时阶段 2 直接跳过、什么都不做**。用户 2026-09-21："如果 stage1
	 * 都跳过了，stage2 应该直接跳过，不要在做任何的动作了，包括 cpm 打印都不
	 * 需要了。"
	 *
	 * `s_host_seen` 是 static（**阶段 1 见过主机**才置 1；且**阶段 1 入口已清零**，
	 * 见上面那段 —— 不清的话 BSS 里的随机值会让这里无条件放行）⇒ 它还是 0，
	 * 就说明这次上电根本没人来测：`READY2` / `[ds] …` / CPM dump / 自检
	 * **全都毫无意义**，直接回启动流程（顺带少打几千字节串口，也能省一点时间）。
	 */
	if (p2 && !s_host_seen)
		return;

	if (p2)
		o_str("READY2 ");
	else
		o_str("READY ");
	o_dec(STUB_VER);
	o_str(" tag=");
	o_hex(DDR_PARAM_LAYOUT_TAG, 4);
	o_ch('/');
	o_hex(CLK_PARAM_LAYOUT_TAG, 4);
	o_str(" words=");
	o_dec(DDR_PARAM_WORDS);
	o_ch('/');
	o_dec(CLK_PARAM_WORDS);
	if (p2)
		o_str(" ddr=ok");	/* 能走到这里 = sdram_init() 没卡死 */
	o_nl();
	/* 等主机的窗口（**毫秒**）—— 排查"是不是窗口太短错过了" */
	o_str("[ds] phase=");
	o_dec((unsigned int)phase);
	o_str(" idle_ms_init=");
	o_dec(DS_IDLE_MS_INIT);
	o_str(" idle_ms_conn=");
	o_dec(DS_IDLE_MS_CONN);
	o_str(" ddr_size=");
	o_dec(DDR_PARAM_SIZE);
	o_str(" clk_size=");
	o_dec(CLK_PARAM_SIZE);
	if (!p2)
		o_str(" state=reset");	/* 阶段 1 入口刚把 static 清零（BSS 还没清）*/
	o_nl();

	/*
	 * 阶段 2 补跑自检：阶段 1 的 `INIT` 若带了 SELFTEST，那时**跑不了**
	 * （时钟还没起来），这里 DDR 已经能用，正好补上。
	 * `seq=0` = "不是对某条命令的应答"，上位机据此识别。
	 */
	/*
	 * ⭐ 阶段 2 先 dump 一遍寄存器（用户 2026-09-21："在阶段2的时候，应该先 dump
	 * 一下 cpm 的寄存器，方便我进行检查，看是否生效……只不过是在阶段 2 的时候，
	 * 发一下 dump 的命令"）。
	 *
	 * **直接复用 `ddr.c` 里已经实现的 `ddr_dump_region()`** —— 就是主机发
	 * `CMD_DUMP` 时走的同一条路，不另写一份 dump：
	 *   `ddr_dump_region(0)` → CPM（PLL/CPCCR/DDRCDR…，`DDR_DUMP_CPM_SIZE`=0x264）。
	 * 想看 PHY(1)/DDRC(2) 时主机随时可以发 `CMD_DUMP`；要开机就全 dump 就把这里
	 * 换成 `ddr_dump_all()`。
	 */
	if (p2)
		ddr_dump_region(0);
	if (p2 && s_want_selftest) {
		s_want_selftest = 0;
		o_str("[ds] phase2 selftest addr=");
		o_hex(SELFTEST_ADDR, 8);
		o_str(" (uncached) len=");
		o_dec(SELFTEST_LEN);
		o_nl();
		(void)memtest_run(MEM_MODE_RW, SELFTEST_ADDR, SELFTEST_LEN,
				  0x5a5a5a5au);
		memtest_reply(0, SELFTEST_LEN);
	}

	for (;;) {
		/*
		 * 阶段 2：能走到这里说明**阶段 1 见过主机**（`s_host_seen`；脱机的在函数
		 * 开头就 return 了）⇒ 给一个窗口让主机发 MEMTEST/BOOT，等不到就正常
		 * 收尾回启动流程。
		 * ⚠️ 阶段 2 是**新一次函数调用**（`debug_stub_run(2)`），局部 `connected`
		 *    从 0 重新开始 —— 所以这里**不能**用它判断，要用 `s_host_seen`
		 *    （static、跨调用保留）。原来就是踩了这个坑（阶段 2 退化成"没连过"
		 *    的长窗口，白等 24s 还打一句 `TIMEOUT phase2 no-host` 让人以为出错）。
		 */
		if (p2)
			used_ms = DS_IDLE_MS_PHASE2;
		else
			used_ms = connected ? DS_IDLE_MS_CONN : DS_IDLE_MS_INIT;
		t0 = get_ticks();
		deadline = ds_deadline(used_ms);
		got = read_frame(deadline, &boot, phase);
		if (!got) {
			if (p2)
				o_str("[ds] phase2 idle, boot\n");
			else if (connected)
				o_str("TIMEOUT disconnect, boot\n");
			else
				o_str("TIMEOUT no-host, boot\n");
			/* 等了多久 / 一共摸到几个字节 —— "板子到底有没有收到东西" */
			o_str("[ds] timeout phase=");
			o_dec((unsigned int)phase);
			o_str(" used_ms=");
			o_dec(used_ms);
			o_str(" waited_ms=");
			o_dec(ds_elapsed_ms(t0));
			o_str(" bytes=");
			o_dec(s_got);
			o_nl();
			return;
		}
		if (!connected) {
			connected = 1;
			s_host_seen = 1;	/* 阶段 2 据此决定"要不要等主机" */
			o_str("[ds] host up (after ");
			o_dec(ds_elapsed_ms(t0));
			o_str(" ms)\n");
		}
		if (boot)
			return;			/* 主机要求继续正常启动 */
	}
}
