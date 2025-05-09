/*
 *
 * Copyright (c) 2013 Ingenic Semiconductor Co.,Ltd
 * Author: <kang.zhao@ingenic.com>
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

#include <config.h>
#include <serial.h>
#include <common.h>
#include <lcd.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <asm/types.h>
#include <asm/arch/tcu.h>
#include <asm/arch/gpio.h>
#include <regulator.h>

#include <jz_lcd/jz_dsim.h>
#include <jz_lcd/lcd_fw050.h>
#include <jz_lcd/jz_lcd_v14.h>
#include "../jz_mipi_dsi/jz_mipi_dsih_hal.h"

vidinfo_t panel_info = { 720 , 1280, LCD_BPP, };
extern struct jzfb_config_info lcd_config_info;
struct lcd_fw050_data lcd_fw050_pdata;

struct fb_videomode jzfb1_videomode = {
	.name = "fitipower_fw050-lcd",
	.refresh = 30,
	.xres = 720,
	.yres = 1280,
	.left_margin = 100,
	.right_margin = 100,
	.upper_margin = 30,
	.lower_margin = 20,
	.hsync_len = 10,
	.vsync_len = 2,
	.sync = FB_SYNC_HOR_HIGH_ACT & FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

struct jzfb_tft_config fw050_cfg = {
	.pix_clk_inv = 0,
	.de_dl = 0,
	.sync_dl = 0,
	.color_even = TFT_LCD_COLOR_EVEN_RGB,
	.color_odd = TFT_LCD_COLOR_ODD_RGB,
	.mode = TFT_LCD_MODE_PARALLEL_888,
};

struct jzfb_config_info jzfb1_init_data = {
        .modes = &jzfb1_videomode,
        .lcd_type = LCD_TYPE_MIPI_TFT,
	.bpp = 24,

	.tft_config = &fw050_cfg,
        .pixclk_falling_edge = 0,
        .date_enable_active_low = 0,

        .dither_enable          = 0,
        .dither.dither_red      = 0,
        .dither.dither_green    = 0,
        .dither.dither_blue     = 0,
};

struct dsi_config jz_dsi_config = {
	.max_lanes = 2,
	.max_hs_to_lp_cycles = 100,
	.max_lp_to_hs_cycles = 40,
	.max_bta_cycles = 4095,
	.color_mode_polarity = 1,
	.shut_down_polarity = 1,
};

struct video_config jz_dsi_video_config={
	.no_of_lanes = 2,
	.virtual_channel =0,
	.color_coding = COLOR_CODE_24BIT,
	.video_mode = VIDEO_BURST_WITH_SYNC_PULSES,
	.receive_ack_packets = 0,
	.is_18_loosely = 0,
	.data_en_polarity = 1,
	.byte_clock = 0,
	.byte_clock_coef = MIPI_PHY_BYTE_CLK_COEF_MUL3_DIV2,
	.lane0_pn_swap = 0,
	.lane1_pn_swap = 0,
	.lane2_pn_swap = 0,
	.lane3_pn_swap = 0,
	.clk_lane_pn_swap = 0,
	.ths_trail_value = 0x10,
};

struct dsi_device jz_dsi = {
	.dsi_config = &jz_dsi_config,
	.video_config = &jz_dsi_video_config,
	.data_lane = 2,
	.bpp_info = 24,
	.max_bps =2750,
};

struct lcd_fw050_data lcd_fw050_pdata = {
	.gpio_lcd_vdd = CONFIG_GPIO_LCD_VDD,
	.gpio_lcd_rst = CONFIG_GPIO_LCD_RST,
	.gpio_lcd_pwm  = CONFIG_GPIO_LCD_PWM,

};

static struct dsi_cmd_packet fitipower_fw050_720_1280_cmd_list[] =
{
	{0x39, 0x04, 0x00, {0xFF, 0x98, 0x81, 0x03}},
	{0x39, 0x02, 0x00, {0x01, 0x00}},
	{0x39, 0x02, 0x00, {0x02, 0x00}},
	{0x39, 0x02, 0x00, {0x03, 0x73}},
	{0x39, 0x02, 0x00, {0x04, 0x00}},
	{0x39, 0x02, 0x00, {0x05, 0x00}},
	{0x39, 0x02, 0x00, {0x06, 0x0A}},
	{0x39, 0x02, 0x00, {0x07, 0x00}},
	{0x39, 0x02, 0x00, {0x08, 0x00}},
	{0x39, 0x02, 0x00, {0x09, 0x01}},
	{0x39, 0x02, 0x00, {0x0A, 0x00}},
	{0x39, 0x02, 0x00, {0x0B, 0x00}},
	{0x39, 0x02, 0x00, {0x0C, 0x01}},
	{0x39, 0x02, 0x00, {0x0D, 0x00}},
	{0x39, 0x02, 0x00, {0x0E, 0x00}},
	{0x39, 0x02, 0x00, {0x0F, 0x1D}},
	{0x39, 0x02, 0x00, {0x10, 0x1D}},
	{0x39, 0x02, 0x00, {0x11, 0x00}},
	{0x39, 0x02, 0x00, {0x12, 0x00}},
	{0x39, 0x02, 0x00, {0x13, 0x00}},
	{0x39, 0x02, 0x00, {0x14, 0x00}},
	{0x39, 0x02, 0x00, {0x15, 0x00}},
	{0x39, 0x02, 0x00, {0x16, 0x00}},
	{0x39, 0x02, 0x00, {0x17, 0x00}},
	{0x39, 0x02, 0x00, {0x18, 0x00}},
	{0x39, 0x02, 0x00, {0x19, 0x00}},
	{0x39, 0x02, 0x00, {0x1A, 0x00}},
	{0x39, 0x02, 0x00, {0x1B, 0x00}},
	{0x39, 0x02, 0x00, {0x1C, 0x00}},
	{0x39, 0x02, 0x00, {0x1D, 0x00}},
	{0x39, 0x02, 0x00, {0x1E, 0x40}},
	{0x39, 0x02, 0x00, {0x1F, 0x80}},
	{0x39, 0x02, 0x00, {0x20, 0x06}},
	{0x39, 0x02, 0x00, {0x21, 0x02}},
	{0x39, 0x02, 0x00, {0x22, 0x00}},
	{0x39, 0x02, 0x00, {0x23, 0x00}},
	{0x39, 0x02, 0x00, {0x24, 0x00}},
	{0x39, 0x02, 0x00, {0x25, 0x00}},
	{0x39, 0x02, 0x00, {0x26, 0x00}},
	{0x39, 0x02, 0x00, {0x27, 0x00}},
	{0x39, 0x02, 0x00, {0x28, 0x33}},
	{0x39, 0x02, 0x00, {0x29, 0x03}},
	{0x39, 0x02, 0x00, {0x2A, 0x00}},
	{0x39, 0x02, 0x00, {0x2B, 0x00}},
	{0x39, 0x02, 0x00, {0x2C, 0x00}},
	{0x39, 0x02, 0x00, {0x2D, 0x00}},
	{0x39, 0x02, 0x00, {0x2E, 0x00}},
	{0x39, 0x02, 0x00, {0x2F, 0x00}},
	{0x39, 0x02, 0x00, {0x30, 0x00}},
	{0x39, 0x02, 0x00, {0x31, 0x00}},
	{0x39, 0x02, 0x00, {0x32, 0x00}},
	{0x39, 0x02, 0x00, {0x33, 0x00}},
	{0x39, 0x02, 0x00, {0x34, 0x04}},
	{0x39, 0x02, 0x00, {0x35, 0x00}},
	{0x39, 0x02, 0x00, {0x36, 0x00}},
	{0x39, 0x02, 0x00, {0x37, 0x00}},
	{0x39, 0x02, 0x00, {0x38, 0x3C}},
	{0x39, 0x02, 0x00, {0x39, 0x00}},
	{0x39, 0x02, 0x00, {0x3A, 0x40}},
	{0x39, 0x02, 0x00, {0x3B, 0x40}},
	{0x39, 0x02, 0x00, {0x3C, 0x00}},
	{0x39, 0x02, 0x00, {0x3D, 0x00}},
	{0x39, 0x02, 0x00, {0x3E, 0x00}},
	{0x39, 0x02, 0x00, {0x3F, 0x00}},
	{0x39, 0x02, 0x00, {0x40, 0x00}},
	{0x39, 0x02, 0x00, {0x41, 0x00}},
	{0x39, 0x02, 0x00, {0x42, 0x00}},
	{0x39, 0x02, 0x00, {0x43, 0x00}},
	{0x39, 0x02, 0x00, {0x44, 0x00}},

	{0x39, 0x02, 0x00, {0x50, 0x01}},
	{0x39, 0x02, 0x00, {0x51, 0x23}},
	{0x39, 0x02, 0x00, {0x52, 0x45}},
	{0x39, 0x02, 0x00, {0x53, 0x67}},
	{0x39, 0x02, 0x00, {0x54, 0x89}},
	{0x39, 0x02, 0x00, {0x55, 0xAB}},
	{0x39, 0x02, 0x00, {0x56, 0x01}},
	{0x39, 0x02, 0x00, {0x57, 0x23}},
	{0x39, 0x02, 0x00, {0x58, 0x45}},
	{0x39, 0x02, 0x00, {0x59, 0x67}},
	{0x39, 0x02, 0x00, {0x5A, 0x89}},
	{0x39, 0x02, 0x00, {0x5B, 0xAB}},
	{0x39, 0x02, 0x00, {0x5C, 0xCD}},
	{0x39, 0x02, 0x00, {0x5D, 0xEF}},

	{0x39, 0x02, 0x00, {0x5E, 0x11}},
	{0x39, 0x02, 0x00, {0x5F, 0x01}},
	{0x39, 0x02, 0x00, {0x60, 0x00}},
	{0x39, 0x02, 0x00, {0x61, 0x15}},
	{0x39, 0x02, 0x00, {0x62, 0x14}},
	{0x39, 0x02, 0x00, {0x63, 0x0E}},
	{0x39, 0x02, 0x00, {0x64, 0x0F}},
	{0x39, 0x02, 0x00, {0x65, 0x0C}},
	{0x39, 0x02, 0x00, {0x66, 0x0D}},
	{0x39, 0x02, 0x00, {0x67, 0x06}},
	{0x39, 0x02, 0x00, {0x68, 0x02}},
	{0x39, 0x02, 0x00, {0x69, 0x07}},
	{0x39, 0x02, 0x00, {0x6A, 0x02}},
	{0x39, 0x02, 0x00, {0x6B, 0x02}},
	{0x39, 0x02, 0x00, {0x6C, 0x02}},
	{0x39, 0x02, 0x00, {0x6D, 0x02}},
	{0x39, 0x02, 0x00, {0x6E, 0x02}},
	{0x39, 0x02, 0x00, {0x6F, 0x02}},
	{0x39, 0x02, 0x00, {0x70, 0x02}},
	{0x39, 0x02, 0x00, {0x71, 0x02}},
	{0x39, 0x02, 0x00, {0x72, 0x02}},
	{0x39, 0x02, 0x00, {0x73, 0x02}},
	{0x39, 0x02, 0x00, {0x74, 0x02}},
	{0x39, 0x02, 0x00, {0x75, 0x01}},
	{0x39, 0x02, 0x00, {0x76, 0x00}},
	{0x39, 0x02, 0x00, {0x77, 0x14}},
	{0x39, 0x02, 0x00, {0x78, 0x15}},
	{0x39, 0x02, 0x00, {0x79, 0x0E}},
	{0x39, 0x02, 0x00, {0x7A, 0x0F}},
	{0x39, 0x02, 0x00, {0x7B, 0x0C}},
	{0x39, 0x02, 0x00, {0x7C, 0x0D}},
	{0x39, 0x02, 0x00, {0x7D, 0x06}},
	{0x39, 0x02, 0x00, {0x7E, 0x02}},
	{0x39, 0x02, 0x00, {0x7F, 0x07}},
	{0x39, 0x02, 0x00, {0x80, 0x02}},
	{0x39, 0x02, 0x00, {0x81, 0x02}},
	{0x39, 0x02, 0x00, {0x82, 0x02}},
	{0x39, 0x02, 0x00, {0x83, 0x02}},
	{0x39, 0x02, 0x00, {0x84, 0x02}},
	{0x39, 0x02, 0x00, {0x85, 0x02}},
	{0x39, 0x02, 0x00, {0x86, 0x02}},
	{0x39, 0x02, 0x00, {0x87, 0x02}},
	{0x39, 0x02, 0x00, {0x88, 0x02}},
	{0x39, 0x02, 0x00, {0x89, 0x02}},
	{0x39, 0x02, 0x00, {0x8A, 0x02}},
	{0x39, 0x04, 0x00, {0xFF, 0x98, 0x81, 0x04}},

	{0x39, 0x02, 0x00, {0x00, 0x80}}, //set 2 line mode

	{0x39, 0x02, 0x00, {0x6C, 0x15}},
	{0x39, 0x02, 0x00, {0x6E, 0x2A}},
	{0x39, 0x02, 0x00, {0x6F, 0x33}},
	{0x39, 0x02, 0x00, {0x8D, 0x14}},
	{0x39, 0x02, 0x00, {0x87, 0xBA}},
	{0x39, 0x02, 0x00, {0x26, 0x76}},
	{0x39, 0x02, 0x00, {0xB2, 0xD1}},
	{0x39, 0x02, 0x00, {0xB5, 0x06}},
	{0x39, 0x02, 0x00, {0x3A, 0x24}},
	{0x39, 0x02, 0x00, {0x35, 0x1F}},
	{0x39, 0x02, 0x00, {0x32, 0x05}},
	{0x39, 0x02, 0x00, {0x33, 0x00}},
	{0x39, 0x02, 0x00, {0x7A, 0x0F}},
	{0x39, 0x04, 0x00, {0xFF, 0x98, 0x81, 0x01}},
	{0x39, 0x02, 0x00, {0x22, 0x0A}},
	{0x39, 0x02, 0x00, {0x31, 0x00}},
	{0x39, 0x02, 0x00, {0x53, 0x9c}},
	{0x39, 0x02, 0x00, {0x55, 0x9c}},
	{0x39, 0x02, 0x00, {0xB7, 0x03}},/*software set lansel pin */

	{0x39, 0x02, 0x00, {0x50, 0xA0}},
	{0x39, 0x02, 0x00, {0x51, 0xA0}},

	{0x39, 0x02, 0x00, {0x60, 0x22}},
	{0x39, 0x02, 0x00, {0x61, 0x00}},
	{0x39, 0x02, 0x00, {0x62, 0x19}},
	{0x39, 0x02, 0x00, {0x63, 0x00}},
	{0x39, 0x02, 0x00, {0xA0, 0x08}},
	{0x39, 0x02, 0x00, {0xA1, 0x19}},
	{0x39, 0x02, 0x00, {0xA2, 0x20}},
	{0x39, 0x02, 0x00, {0xA3, 0x12}},
	{0x39, 0x02, 0x00, {0xA4, 0x13}},
	{0x39, 0x02, 0x00, {0xA5, 0x24}},
	{0x39, 0x02, 0x00, {0xA6, 0x19}},
	{0x39, 0x02, 0x00, {0xA7, 0x19}},
	{0x39, 0x02, 0x00, {0xA8, 0x6b}},
	{0x39, 0x02, 0x00, {0xA9, 0x1a}},
	{0x39, 0x02, 0x00, {0xAA, 0x27}},
	{0x39, 0x02, 0x00, {0xAB, 0x62}},
	{0x39, 0x02, 0x00, {0xAC, 0x18}},
	{0x39, 0x02, 0x00, {0xAD, 0x15}},
	{0x39, 0x02, 0x00, {0xAE, 0x4c}},
	{0x39, 0x02, 0x00, {0xAF, 0x22}},
	{0x39, 0x02, 0x00, {0xB0, 0x25}},
	{0x39, 0x02, 0x00, {0xB1, 0x5b}},
	{0x39, 0x02, 0x00, {0xB2, 0x6f}},
	{0x39, 0x02, 0x00, {0xB3, 0x39}},

	{0x39, 0x02, 0x00, {0xC0, 0x08}},
	{0x39, 0x02, 0x00, {0xC1, 0x0d}},
	{0x39, 0x02, 0x00, {0xC2, 0x19}},
	{0x39, 0x02, 0x00, {0xC3, 0x0C}},
	{0x39, 0x02, 0x00, {0xC4, 0x0c}},
	{0x39, 0x02, 0x00, {0xC5, 0x1F}},
	{0x39, 0x02, 0x00, {0xC6, 0x13}},
	{0x39, 0x02, 0x00, {0xC7, 0x1a}},
	{0x39, 0x02, 0x00, {0xC8, 0x5d}},
	{0x39, 0x02, 0x00, {0xC9, 0x1b}},
	{0x39, 0x02, 0x00, {0xCA, 0x26}},
	{0x39, 0x02, 0x00, {0xCB, 0x5b}},
	{0x39, 0x02, 0x00, {0xCC, 0x19}},
	{0x39, 0x02, 0x00, {0xCD, 0x18}},
	{0x39, 0x02, 0x00, {0xCE, 0x4C}},
	{0x39, 0x02, 0x00, {0xCF, 0x22}},
	{0x39, 0x02, 0x00, {0xD0, 0x2d}},
	{0x39, 0x02, 0x00, {0xD1, 0x5D}},
	{0x39, 0x02, 0x00, {0xD2, 0x6b}},
	{0x39, 0x02, 0x00, {0xD3, 0x39}},

	{0x39, 0x04, 0x00, {0xFF, 0x98, 0x81, 0x00}},
};

void panel_pin_init(void)
{
	int ret = 0;
	ret = gpio_request(lcd_fw050_pdata.gpio_lcd_vdd,"lcd_vdd");
	if(ret){
	 	/*printf("cannot request gpoi lcd_vdd\n");*/
	}
	ret = gpio_request(lcd_fw050_pdata.gpio_lcd_rst,"lcd_rst");
	if(ret){
	 	/*printf("cannot request gpoi lcd_rst\n");*/
	}

	ret = gpio_request(lcd_fw050_pdata.gpio_lcd_pwm,"lcd_pwm");
	if(ret){
	 	/*printf("cannot request gpoi lcd_pwm\n");*/
	}

	serial_puts("lcd_fw050 panel display pin init\n");

}

void panel_power_on(void)
{
	gpio_direction_output(lcd_fw050_pdata.gpio_lcd_vdd,0);
	mdelay(50);
	gpio_direction_output(lcd_fw050_pdata.gpio_lcd_rst,0);
	mdelay(10);
	gpio_direction_output(lcd_fw050_pdata.gpio_lcd_rst,1);
#ifndef CONFIG_X2600
	gpio_direction_output(lcd_fw050_pdata.gpio_lcd_pwm,1);
#endif
	serial_puts("lcd_fw050 panel display on\n");
}
#ifdef CONFIG_X2600
void panel_pwm_on(void)
{
	mdelay(100);
	gpio_direction_output(lcd_fw050_pdata.gpio_lcd_pwm,1);
}
#endif
void panel_power_off(void)
{
	gpio_direction_output(lcd_fw050_pdata.gpio_lcd_vdd,1);
	gpio_direction_output(lcd_fw050_pdata.gpio_lcd_rst,0);
	gpio_direction_output(lcd_fw050_pdata.gpio_lcd_pwm,0);
	serial_puts("lcd_fw050 panel display off\n");

}

void fw050_sleep_in(struct dsi_device *dsi)
{
	struct dsi_cmd_packet data_to_send = {0x05, 0x10, 0x00};
	write_command(dsi, data_to_send);
}

void fw050_sleep_out(struct dsi_device *dsi)
{
	struct dsi_cmd_packet data_to_send = {0x05, 0x11, 0x00};
	write_command(dsi, data_to_send);
}

void fw050_display_on(struct dsi_device *dsi)
{
	struct dsi_cmd_packet data_to_send = {0x05, 0x29, 0x00};
	write_command(dsi, data_to_send);
}

void fw050_display_off(struct dsi_device *dsi)
{
	struct dsi_cmd_packet data_to_send = {0x05, 0x28, 0x00};
	write_command(dsi, data_to_send);

}

void fw050_panel_init(struct dsi_device *dsi)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(fitipower_fw050_720_1280_cmd_list); i++)
	{
		write_command(dsi, fitipower_fw050_720_1280_cmd_list[i]);
	}
}

void panel_init_sequence(struct dsi_device *dsi)
{
	fw050_panel_init(dsi);
	fw050_sleep_out(dsi);
	mdelay(120);
	fw050_display_on(dsi);
	mdelay(5);
}


