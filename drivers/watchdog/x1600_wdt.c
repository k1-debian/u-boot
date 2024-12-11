#include <common.h>
#include <asm/arch/clk.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>

#define WDT_FULL            0x0
#define WDT_ENABLE          0x4
#define WDT_COUNT           0x8
#define WDT_CONTROL         0xC
#define TCU_TSSTR           0x2C
#define TCU_TSCLR           0x3C

#define TCEN                0

#define WDT_MAX_COUNT       (0xFFFF)

#define RTC_EN              1
#define PRESCALE            3
#define CLRZ                10
#define WDT_CLK_DIV_1       0
#define WDT_CLK_DIV_4       1
#define WDT_CLK_DIV_16      2
#define WDT_CLK_DIV_64      3
#define WDT_CLK_DIV_256     4
#define WDT_CLK_DIV_1024    5

#define WDTSS               16
#define WDTSC               16
#define WDT_IOBASE         0xb0002000

#define OPCR_ERCS_BIT       2
#define CLKGR_TCU_BIT       18

static inline void wdt_write_reg(unsigned int reg, unsigned int value)
{
    writel(value, WDT_IOBASE+reg);
}

static inline unsigned int wdt_read_reg(unsigned int reg)
{
    return readl(WDT_IOBASE + reg);
}

static inline void wdt_set_bit(unsigned int reg, int bit, unsigned int val)
{
    unsigned int reg_val = wdt_read_reg(reg);
    if (val == 1)
        reg_val |= (1<<bit);
    else
        reg_val &= ~(1<<bit);

    wdt_write_reg(reg, reg_val);
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
    unsigned int us;
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

    wdt_set_bit(WDT_ENABLE, TCEN, 0);

    unsigned int val = (clock_div << PRESCALE) | (1 << CLRZ) | (1 << RTC_EN);
    wdt_write_reg(WDT_CONTROL, val);

    wdt_write_reg(WDT_FULL, count);

    return 0;
}


int wdt_start(unsigned long ms)
{
    wdt_set_bit(TCU_TSCLR, WDTSC, 1); //使能看门狗计数器

    if (jz_wdt_set_timeout(ms)) {
        serial_debug("wdt set time error\n");
        return -1;
    }

    wdt_set_bit(WDT_ENABLE, TCEN, 1);

    return 0;
}

int wdt_init(void)
{
    cpm_clear_bit(CLKGR_TCU_BIT, CPM_CLKGR0);     // open tcu clk
    return 0;
}