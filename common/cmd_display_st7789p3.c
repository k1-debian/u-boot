/*
 * U-Boot command for ST7789P3 LCD initialization and logo display
 * 基于Linux驱动nemc-panel-st7789p3.c的初始化序列
 */

#include <common.h>
#include <command.h>
#include <lcd.h>
#include <asm/arch/gpio.h>
#include <asm/arch-x2600/cpm.h>
#include <asm/arch-x2600/spinand.h>
#include <asm/io.h>
#include <spi_flash.h>
#include <malloc.h>

/* ST7789P3屏幕定义 */
#define LCD_W 240
#define LCD_H 320

/* NEMC基地址  */
#define NEMC_PADDR 0x1b000000
#define NEMC_SIZE 0x1000000

/* GPIO定义  */
#define GPIO_LCD_TE     GPIO_PB(0)
#define GPIO_LCD_RS     GPIO_PB(1)
#define GPIO_LCD_RST    GPIO_PB(2)
#define GPIO_LCD_CS     GPIO_PB(3)
#define GPIO_LCD_LED_EN GPIO_PC(7)
//GPIO_PB(28)

/* Logo在Flash中的存储地址和大小 */
#define LOGO_SIZE (LCD_W * LCD_H * 2)



/* 全局变量 */
static int lcd_initialized = 0;
static void *nemc_base_addr = (void *)(0xbb000000);
/* 初始化数据结构 - */
struct panel_init_data {
    u8 cmd;
    u8 *data;
    u8 data_len;
};

#define INIT_ENTRY(c, ...)                                             \
    {                                                              \
        .cmd = c, .data = (u8[]){ __VA_ARGS__ },               \
        .data_len = sizeof((u8[]){ __VA_ARGS__ }) / sizeof(u8) \
    }

/* RT20QV227A初始化序列  */
static struct panel_init_data st7789p3_init_data[] = {
    INIT_ENTRY(0xB2, 0x0C, 0x0C, 0x00, 0x33, 0x33),
    INIT_ENTRY(0x35, 0x00), // TE on
    INIT_ENTRY(0x36, 0x00),
    INIT_ENTRY(0x3A, 0x05),
    INIT_ENTRY(0xB7, 0x74),
    INIT_ENTRY(0xBB, 0x1E),
    INIT_ENTRY(0xC0, 0x2C),
    INIT_ENTRY(0xC2, 0x01),
    INIT_ENTRY(0xC3, 0x19),
    INIT_ENTRY(0xC4, 0x20),
    INIT_ENTRY(0xC6, 0x0F),
    INIT_ENTRY(0xD0, 0xA4, 0xA1),
    INIT_ENTRY(0xD6, 0xA1),
    INIT_ENTRY(0xE0, 0xF0, 0x08, 0x0E, 0x09, 0x08, 0x16, 0x33, 0x43, 0x4A, 0x38, 0x14, 0x14, 0x2E, 0x32),
    INIT_ENTRY(0xE1, 0xF0, 0x0A, 0x10, 0x0A, 0x09, 0x04, 0x33, 0x33, 0x49, 0x0A, 0x16, 0x15, 0x2C, 0x30),
    INIT_ENTRY(0x21),
};

/* 发送命令到LCD -  */
static void send_cmd(u8 cmd)
{
    volatile u16 *addr = (volatile u16 *)nemc_base_addr;

    if (!addr) {
        printf("Error: NEMC address not initialized\n");
        return;
    }

    /* 与Linux驱动一致：先设置RS=1，发送命令，然后设置RS=0 */
    gpio_direction_output(GPIO_LCD_RS, 0);  // RS=1
    *addr = cmd << 8;
    udelay(100);
    gpio_direction_output(GPIO_LCD_RS, 1);  // RS=0
    udelay(100);
}

/* 发送数据到LCD  */
static void send_dat(u8 *data, int size)
{
    volatile u16 *addr = (volatile u16 *)nemc_base_addr;
    int i;

    if (!addr) {
        printf("Error: NEMC address not initialized\n");
        return;
    }

    for (i = 0; i < size; i++) {
        *addr = data[i] << 8;
//        printf("==== [%x]\n", data[i]);
        udelay(100);
    }
}

/* 旋转函数定义*/
#define ROTATE_90_DST(x, y) ((LCD_H - (x + 1) * 4) * LCD_W + y)
#define ROTATE_270_DST(x, y) ((x * 4) * LCD_W + (LCD_W - 1 - y))

static void rotate_90_degree(u32 *dst, u32 *src)
{
    int x, y, i;
    for (i = 0; i < (LCD_H / 4) / 8 * 8; i += 8) {
        for (y = 0; y < LCD_W; y++) {
            for (x = i; x < i + 8; x++) {
                u32 s0 = src[y * (LCD_H / 2) + x * 2];
                u32 s1 = src[y * (LCD_H / 2) + x * 2 + 1];
                int dst_index = ROTATE_90_DST(x, y);
                dst[dst_index + 3 * LCD_W] = ((s0 << 8) & 0xff00) | ((s0 << 16) & 0xff000000);
                dst[dst_index + 2 * LCD_W] = ((s0 >> 8) & 0xff00) | (s0 & 0xff000000);
                dst[dst_index + 1 * LCD_W] = ((s1 << 8) & 0xff00) | ((s1 << 16) & 0xff000000);
                dst[dst_index + 0 * LCD_W] = ((s1 >> 8) & 0xff00) | (s1 & 0xff000000);
            }
        }
    }
    if (i < LCD_H / 4) {
        for (y = 0; y < LCD_W; y++) {
            for (x = i; x < LCD_H / 4; x++) {
                u32 s0 = src[y * (LCD_H / 2) + x * 2];
                u32 s1 = src[y * (LCD_H / 2) + x * 2 + 1];
                int dst_index = ROTATE_90_DST(x, y);
                dst[dst_index + 3 * LCD_W] = ((s0 << 8) & 0xff00) | ((s0 << 16) & 0xff000000);
                dst[dst_index + 2 * LCD_W] = ((s0 >> 8) & 0xff00) | (s0 & 0xff000000);
                dst[dst_index + 1 * LCD_W] = ((s1 << 8) & 0xff00) | ((s1 << 16) & 0xff000000);
                dst[dst_index + 0 * LCD_W] = ((s1 >> 8) & 0xff00) | (s1 & 0xff000000);
            }
        }
    }
}

static void rotate_270_degree(u32 *dst, u32 *src)
{
    int x, y, i;
    for (i = 0; i < (LCD_H / 4) / 8 * 8; i += 8) {
        for (y = 0; y < LCD_W; y++) {
            for (x = i; x < i + 8; x++) {
                u32 s0 = src[y * (LCD_H / 2) + x * 2];
                u32 s1 = src[y * (LCD_H / 2) + x * 2 + 1];
                int dst_index = ROTATE_270_DST(x, y);
                dst[dst_index + 0 * LCD_W] = ((s0 << 8) & 0xff00) | ((s0 << 16) & 0xff000000);
                dst[dst_index + 1 * LCD_W] = ((s0 >> 8) & 0xff00) | (s0 & 0xff000000);
                dst[dst_index + 2 * LCD_W] = ((s1 << 8) & 0xff00) | ((s1 << 16) & 0xff000000);
                dst[dst_index + 3 * LCD_W] = ((s1 >> 8) & 0xff00) | (s1 & 0xff000000);
            }
        }
    }
    if (i < LCD_H / 4) {
        for (y = 0; y < LCD_W; y++) {
            for (x = i; x < LCD_H / 4; x++) {
                u32 s0 = src[y * (LCD_H / 2) + x * 2];
                u32 s1 = src[y * (LCD_H / 2) + x * 2 + 1];
                int dst_index = ROTATE_270_DST(x, y);
                dst[dst_index + 0 * LCD_W] = ((s0 << 8) & 0xff00) | ((s0 << 16) & 0xff000000);
                dst[dst_index + 1 * LCD_W] = ((s0 >> 8) & 0xff00) | (s0 & 0xff000000);
                dst[dst_index + 2 * LCD_W] = ((s1 << 8) & 0xff00) | ((s1 << 16) & 0xff000000);
                dst[dst_index + 3 * LCD_W] = ((s1 >> 8) & 0xff00) | (s1 & 0xff000000);
            }
        }
    }
}

static void rotate_0_degree(u32 *dst, u32 *src)
{
	int x, y;
	for (y = 0; y < LCD_H; y++) {
		for (x = 0; x < LCD_W / 4; x++) {
			u32 s0 = src[0];
			u32 s1 = src[1];
			dst[0] = ((s0 << 8) & 0xff00) |
				 ((s0 << 16) & 0xff000000);
			dst[1] = ((s0 >> 8) & 0xff00) | (s0 & 0xff000000);
			dst[2] = ((s1 << 8) & 0xff00) |
				 ((s1 << 16) & 0xff000000);
			dst[3] = ((s1 >> 8) & 0xff00) | (s1 & 0xff000000);
			dst += 4;
			src += 2;
		}
	}
}

#define GPIO_PORTB_SET_FUNC1(pin)			\
do {					\
		*((volatile unsigned int *)0xB3602018) |= (1 << pin); \
		*((volatile unsigned int *)0xB3602028) |= (1 << pin); \
		*((volatile unsigned int *)0xB3602038) |= (1 << pin); \
		*((volatile unsigned int *)0xB3602044) |= (1 << pin); \
}while(0)

#define GPIO_PORTB_SET_FUNC3(pin)			\
do {					\
		*((volatile unsigned int *)0xB3602018) |= (1 << pin); \
		*((volatile unsigned int *)0xB3602028) |= (1 << pin); \
		*((volatile unsigned int *)0xB3602034) |= (1 << pin); \
		*((volatile unsigned int *)0xB3602044) |= (1 << pin); \
}while(0)

#define NEMC_REG_ADDR 0xB3410000
#define SMC0R1 0x14
#define SMC1R1 0x54

/* 初始化GPIO引脚 */
static int st7789p3_gpio_init(void)
{
    int ret = 0;

    printf("ST7789P3: Initializing GPIO pins\n");

    /* 请求和配置GPIO */
    gpio_request(GPIO_LCD_RST, "lcd_rst");
    gpio_request(GPIO_LCD_RS, "lcd_rs");
    gpio_request(GPIO_LCD_CS, "lcd_cs");

    gpio_request(GPIO_LCD_LED_EN, "lcd_led_en");

    gpio_request(GPIO_LCD_TE, "lcd_te");
    gpio_direction_input(GPIO_LCD_TE);

    GPIO_PORTB_SET_FUNC3((20));
    GPIO_PORTB_SET_FUNC3((21));
    GPIO_PORTB_SET_FUNC3((22));
    GPIO_PORTB_SET_FUNC3((23));
    GPIO_PORTB_SET_FUNC3((24));
    GPIO_PORTB_SET_FUNC3((25));
    GPIO_PORTB_SET_FUNC3((26));
    GPIO_PORTB_SET_FUNC3((27));
    GPIO_PORTB_SET_FUNC3((29));

    dump_gpio_func(GPIO_PB(20));
    dump_gpio_func(GPIO_PB(10));
    dump_gpio_func(GPIO_PB(23));
    dump_gpio_func(GPIO_PB(29));

    // 设置16bit输出
    u32 val = 0;
    val = readl(NEMC_REG_ADDR+ SMC0R1);
	val = 0x020c0b00; // 所有读时序全部为0 WE
    val &= ~(3 << 6);
    val |= 1 << 6;
    writel(val, NEMC_REG_ADDR+ SMC0R1);
    val = 0;
    writel(val, NEMC_REG_ADDR+ SMC1R1);

    return 0;
}

static int st7789p3_power_on(void)
{
    printf("ST7789P3: Powering on LCD\n");

    /* 启用背光 */
    gpio_direction_output(GPIO_LCD_LED_EN, 0);

    gpio_direction_output(GPIO_LCD_RST, 1);
    mdelay(10);
    gpio_direction_output(GPIO_LCD_RST, 0);
    mdelay(100);
    gpio_direction_output(GPIO_LCD_RST, 1);

    gpio_direction_output(GPIO_LCD_CS, 0);
}

/* 退出睡眠模式 */
static void st7789p3_exit_sleep(void)
{
    send_cmd(0x11);
    mdelay(120);
    send_cmd(0x29);
}

/* 初始化LCD IC - 与Linux驱动完全一致 */
static int st7789p3_panel_init(void)
{
    int i;

    printf("ST7789P3: Initializing panel IC\n");

    /* 发送初始化序列 */
    for (i = 0; i < ARRAY_SIZE(st7789p3_init_data); i++) {
        printf("ST7789P3: Sending command 0x%02x\n", st7789p3_init_data[i].cmd);
        send_cmd(st7789p3_init_data[i].cmd);

        if (st7789p3_init_data[i].data_len > 0) {
            send_dat(st7789p3_init_data[i].data, st7789p3_init_data[i].data_len);
        }

        udelay(200);
    }

    /* 退出睡眠模式 */
    st7789p3_exit_sleep();

    printf("ST7789P3: Panel initialized\n");
    return 0;
}

/* 完整的LCD初始化 */
static int st7789p3_lcd_init(void)
{
    if (lcd_initialized) {
        printf("ST7789P3: LCD already initialized\n");
        return 0;
    }

    printf("ST7789P3: Starting LCD initialization\n");
    printf("ST7789P3: NEMC base address: 0x%08lx\n", (ulong)nemc_base_addr);

    if (!nemc_base_addr) {
        printf("ST7789P3: Error: NEMC address is NULL\n");
        return -1;
    }

    if (st7789p3_gpio_init() != 0) {
        printf("ST7789P3: Error: GPIO initialization failed\n");
        return -1;
    }

    /* 上电 */
    if (st7789p3_power_on() != 0) {
        printf("ST7789P3: Error: Power on failed\n");
        return -1;
    }

    /* 初始化IC */
    if (st7789p3_panel_init() != 0) {
        printf("ST7789P3: Error: Panel initialization failed\n");
        return -1;
    }
	if(1) {
		// 设置16bit输出
		u32 val = 0;
		val = 0x020c0b00; // 所有读时序全部为0 WE
				  // 底电平为0xc,高电平由0x2,0xb
		val &= ~(3 << 6);
		val |= 1 << 6;

        writel(val, NEMC_REG_ADDR+ SMC0R1);
	}

    send_cmd(0x2C);
    char a = 0;
    send_dat(&a, 1);

    lcd_initialized = 1;
    printf("ST7789P3: LCD initialization completed\n");

    return 0;
}

static void st7789p3_display_test(int rotate_angle)
{
    volatile u16 *addr = (volatile u16 *)nemc_base_addr;
    int i;

    printf("ST7789P3: Displaying test pattern with rotation %d degrees\n", rotate_angle);

    if (!addr) {
        printf("ST7789P3: Error: NEMC address not initialized\n");
        return;
    }

    send_cmd(0x2C);

    //gpio_direction_output(GPIO_LCD_CS, 1);
    //gpio_direction_output(GPIO_LCD_CS, 0);

    for (i = 0; i < LCD_W * LCD_H *2; i++) {
        *addr = 0xf8 << 8;
    }

    //gpio_direction_output(GPIO_LCD_CS, 1);
    printf("ST7789P3: Test pattern display completed\n");
}

struct _logo_info {
	int width;
	int height;
	int bpp;
	unsigned int p8;
	unsigned int background_color;
} __attribute__ ((packed));


void bgra8888_to_rgb565_optimized(const uint8_t *src, uint16_t *dst, size_t pixel_count) {
    if (src == NULL || dst == NULL || pixel_count == 0) return;

    int i = 0;
    for (i = 0; i < pixel_count; i++) {
        uint8_t b = src[i * 4 + 0];  // 蓝色
        uint8_t g = src[i * 4 + 1];  // 绿色
        uint8_t r = src[i * 4 + 2];  // 红色

        dst[i] = ((uint16_t)(r & 0xF8) << 8) | 
            ((uint16_t)(g & 0xFC) << 3) | 
            ((uint16_t)(b & 0xF8) >> 3);
    }
}

static void st7789p3_display_logo(int rotate_angle)
{
    printf("ST7789P3: Displaying logo from flash with rotation %d degrees\n", rotate_angle);

    printf("ST7789P3: Using CPU send data method for logo\n");
    volatile struct _logo_info *pinfo = (volatile struct _logo_info *)(0x81800000);
    printf("Logo info: %d, %d, %d, 0x%x\n", pinfo->width, pinfo->height, pinfo->bpp, pinfo->background_color);

    if (!pinfo || pinfo->width <= 0 || pinfo->height <= 0) {
        printf("ST7789P3: Invalid logo info\n");
        gpio_direction_output(GPIO_LCD_CS, 1);
        return;
    }

    int width = pinfo->width;
    int height = pinfo->height;
    uint32_t bpp = pinfo->bpp;
    unsigned int background_color = pinfo->background_color;
    volatile u16 *addr = (volatile u16 *)nemc_base_addr;

    int w = LCD_W;
    int h = LCD_H;

    int screen_w, screen_h;
    if (rotate_angle == 90) {
        screen_w = LCD_H;
        screen_h = LCD_W;
    } else {
        screen_w = LCD_W;
        screen_h = LCD_H;
    }
    printf("ST7789P3: Logo %dx%d, Screen %dx%d \n",
            width, height, w, h);

    /* 分配屏幕缓冲区*/
    size_t buf_size = w * h * 4;
    u32 *screen_buffer = malloc(buf_size);
    if (!screen_buffer) {
        printf("ST7789P3: Error: Failed to allocate screen buffer\n");
        gpio_direction_output(GPIO_LCD_CS, 1);
        return;
    }

    int i, j;
    int y, x;
    u16* sbuf = screen_buffer;
    u32 *dest = malloc(buf_size);
    if (!dest) {
        printf("ST7789P3: Error: Failed to allocate dest buffer\n");
        gpio_direction_output(GPIO_LCD_CS, 1);
        return;
    }

    u16 *d = (u16*)(dest);
    memset(sbuf, background_color, w * h * 4);

    u8 *logo_data = (u8*)((char *)pinfo + sizeof(struct _logo_info));
    bgra8888_to_rgb565_optimized(logo_data, d, width * height);

    if (rotate_angle == 90 ) {
        rotate_90_degree(sbuf, dest);
    }

    d = (u16*)(dest);
    for ( i = 0; i < w * h *2; i++) {
        *addr = sbuf[i];
    }

    gpio_direction_output(GPIO_LCD_CS, 1);
    free(screen_buffer);
    free(dest);

    printf("ST7789P3: Logo display completed\n");
}

/* ST7789P3命令处理函数 */
static int do_st7789p3_logo(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    int rotate_angle = 0;

    printf("ST7789P3: Command started\n");


    if (argc == 2) {
        rotate_angle = simple_strtol(argv[1], NULL, 10);
        if (rotate_angle != 0 && rotate_angle != 90 && rotate_angle != 270) {
            printf("ST7789P3: Error: Invalid rotation angle. Use 0, 90, or 270.\n");
            return CMD_RET_USAGE;
        }
    } else if (argc > 2) {
        printf("Usage: st7789p3 [angle]\n");
        printf("  angle - Rotation angle (0, 90, 270 degrees, default: 0)\n");
        return CMD_RET_USAGE;
    }

    printf("ST7789P3: Starting with rotation %d degrees\n", rotate_angle);


    if (st7789p3_lcd_init() != 0) {
        printf("ST7789P3: Error: LCD initialization failed\n");
        return CMD_RET_FAILURE;
    }

    /* 显示测试画面 */
//    st7789p3_display_test(rotate_angle);
    rotate_angle = 90;
    st7789p3_display_logo(rotate_angle);


    printf("ST7789P3: Command completed successfully\n");
    return CMD_RET_SUCCESS;
}

/* 简化的命令定义 */
U_BOOT_CMD(
    st7789p3, 1, 1, do_st7789p3_logo,
    "ST7789P3 LCD logo display with automatic initialization",
    "No parameters required.\n"
    "Example:\n"
    "  st7789p3      - Display logo with default rotation (e.g., 90 degree)\n"
);
