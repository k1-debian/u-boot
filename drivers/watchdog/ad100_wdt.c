#include <common.h>
#include <asm/arch/clk.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>

#define TCU_TSCLR       0x3C

#define WDT_TDR     0x0
#define WDT_TCER    0x4
#define WDT_TCNT    0x8
#define WDT_TCSR    0xC

#define WDT_CLK_DIV_1       0
#define WDT_CLK_DIV_4       1
#define WDT_CLK_DIV_16      2
#define WDT_CLK_DIV_64      3
#define WDT_CLK_DIV_256     4
#define WDT_CLK_DIV_1024    5

#define WDT_MAX_COUNT       (0xFFFF)
#define RTC_EN              2

#define OPCR_ERCS_BIT       2
#define CLKGR_TCU_BIT       18

#define WDT0_IOBASE         0xb3630000

static inline void wdt_write_reg(unsigned int reg, unsigned int value)
{
    writel(value, WDT0_IOBASE+reg);
}

static inline unsigned int wdt_read_reg(unsigned int reg)
{
    return readl(WDT0_IOBASE + reg);
}

unsigned long get_rtc_internal_clk_rate(void)
{
    unsigned int rtc_32k_is_on = cpm_test_bit(OPCR_ERCS_BIT, CPM_OPCR);

    if (!rtc_32k_is_on)
        return 24000000 / 512;

    return 32768;
}

static int jz_wdt_set_timeout(unsigned long ms)
{
    unsigned int val, us;
    unsigned long count = ms;
    unsigned int clock_div = 0;

    unsigned long rate = get_rtc_internal_clk_rate();

    us = 1000000 / rate;

    count = ms * 1000 / us;

    while (count > WDT_MAX_COUNT) {
        if (clock_div == WDT_CLK_DIV_1024)
            return -1;

        count /= 4;
        clock_div += 1;
    }

    wdt_write_reg(WDT_TCER, 0);

    val = (clock_div << 3) | RTC_EN;

    wdt_write_reg(WDT_TCSR, val);

    wdt_write_reg(WDT_TDR, count);

    wdt_write_reg(WDT_TCNT, 0);

    wdt_write_reg(WDT_TCER, 1);

    return 0;
}

int wdt_start(unsigned long ms)
{
    wdt_write_reg(TCU_TSCLR, 1 << 16);// 使能看门狗计数器

    if (jz_wdt_set_timeout(ms)) {
        serial_debug("wdt set time error\n");
        return -1;
    }

    wdt_write_reg(WDT_TCER, 1);
    return 0;
}

int wdt_init(void)
{
    cpm_clear_bit(CLKGR_TCU_BIT, CPM_CLKGR0);     // open tcu clk
    return 0;
}