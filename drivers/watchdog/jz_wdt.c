/*
  * WDT DRIVER
  *
  * Copyright (c) 2015 Ingenic Semiconductor Co.,Ltd
  * Author: gao wei <wei.gao@ingenic.com>
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License as
  * published by the Free Software Foundation; either version 2 of
  * the License, or (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
  * MA 02111-1307 USA
  */

/* #define DEBUG */
#include <common.h>
#include <watchdog.h>
#include <asm/io.h>
#include <asm/arch/wdt.h>

#define wdt_write(value, reg) writel(value, WDT_BASE + reg)
#define wdt_read(reg) readl(WDT_BASE + reg)

struct {
        int div;
        unsigned int value;
} wdt_div_table[] = {
        {1   , TCSR_PRESCALE_1   },
        {4   , TCSR_PRESCALE_4   },
        {16  , TCSR_PRESCALE_16  },
        {64  , TCSR_PRESCALE_64  },
        {256 , TCSR_PRESCALE_256 },
        {1024, TCSR_PRESCALE_1024},
};

static int wdt_settimeout(unsigned int timeout) /* timeout: ms */
{
        unsigned int i;
        unsigned int reg;
        unsigned int base_freq;
        int wdt_div_num = -1;
        unsigned int tdr, clk_in;

#if defined(CONFIG_WDT_FREQ_BY_RTC)
        base_freq = RTC_FREQ;
        clk_in = TCSR_RTC_EN;
#elif defined(CONFIG_WDT_FREQ_BY_EXCLK)
        base_freq = CONFIG_SYS_EXTAL;
        clk_in = TCSR_EXT_EN;
#elif defined(CONFIG_WDT_FREQ_BY_PCLK)
        goto err;
#else
        goto err;
#endif

        unsigned long counter_min, counter_max;
        for ( i=0; i<sizeof(wdt_div_table); i++ ) {
                counter_min = wdt_div_table[i].div * 0x2 * 1000 / base_freq; //WDT_TDR set should bigger than 0x1
                counter_max = wdt_div_table[i].div * 0xffff * 1000 / base_freq;
                if (timeout >= counter_min
                                && timeout <= counter_max
                                && (base_freq * 2 / wdt_div_table[i].div < CONFIG_SYS_EXTAL)) {
                        wdt_div_num = i;
                        tdr = timeout * base_freq / wdt_div_table[i].div / 1000 + 1;
                        break;
                }
        }

        if (wdt_div_num == -1)
                goto err;

        reg = wdt_read( WDT_TCER);                                              // shutdown wdt frist
        wdt_write(reg & ~TCER_TCEN, WDT_TCER);

        wdt_write(tdr, WDT_TDR);                                                // set contrast count
        wdt_write(clk_in | wdt_div_table[wdt_div_num].value, WDT_TCSR);         // set clk config
        wdt_write(0x0, WDT_TCNT);                                               // clean counter

#ifdef DEBUG
        debug("WDT CLK IN is  %dHZ\n" , base_freq);
        debug("REAL FREQ is   %dHZ\n" , base_freq / wdt_div_table[i].div);
        debug("The timeout is %dms\n" , tdr * 1000 * wdt_div_table[i].div / base_freq);
        debug("WDT_TDR:       0x%x\n" , wdt_read(WDT_TDR));
        debug("WDT_TCSR:      0x%x\n" , wdt_read(WDT_TCSR));
#endif

        reg = wdt_read(WDT_TCER);                                               // restart wdt
        wdt_write(reg | TCER_TCEN, WDT_TCER);

        return 0;
err:
        printf("Unable to provide the timeout, please check it!");
        return -1;
}

void hw_watchdog_disable(void)
{
        wdt_write(wdt_read(WDT_TCER) & ~TCER_TCEN, WDT_TCER);
}

void hw_watchdog_reset(void)
{
        debug("watchdog reset\n");
        wdt_write(0x0, WDT_TCNT);

}
void hw_watchdog_init(void)
{
        wdt_settimeout(CONFIG_WDT_TIMEOUT_BY_MS);
}

