/*
 * X3000 GPIO definitions.
 */
#ifndef __X3000_GPIO_H__
#define __X3000_GPIO_H__

#include <asm/arch/base.h>

#define GPIO_PA(n)	(0 * 32 + (n))
#define GPIO_PB(n)	(1 * 32 + (n))
#define GPIO_PC(n)	(2 * 32 + (n))
#define GPIO_PD(n)	(3 * 32 + (n))
#define GPIO_PE(n)	(4 * 32 + (n))

enum gpio_function {
	GPIO_FUNC_0	= 0x00,
	GPIO_FUNC_1	= 0x01,
	GPIO_FUNC_2	= 0x02,
	GPIO_FUNC_3	= 0x03,
	GPIO_OUTPUT0	= 0x04,
	GPIO_OUTPUT1	= 0x05,
	GPIO_INPUT	= 0x06,
	GPIO_RISE_EDGE	= 0x0b,
	GPIO_PULLUP	= 0x10,
	GPIO_PULLDOWN	= 0x20,
	GPIO_PULL_HIZ	= 0x40,
	GPIO_1_8V	= 0x80,
};

enum gpio_port {
	GPIO_PORT_A,
	GPIO_PORT_B,
	GPIO_PORT_C,
	GPIO_PORT_D,
	GPIO_PORT_E,
	GPIO_NR_PORTS,
};

enum gpio_driver_strength {
	GPIO_DS_LEVEL_INVALID = -1,
	GPIO_DS_LEVEL_0 = 0x0,
	GPIO_DS_LEVEL_1 = 0x1,
	GPIO_DS_LEVEL_2 = 0x2,
	GPIO_DS_LEVEL_3 = 0x3,
};

struct jz_gpio_func_def {
	int port;
	int func;
	unsigned long pins;
};

#define MAX_GPIO_NUM	160

#define PXPIN		0x00
#define PXINT		0x10
#define PXINTS		0x14
#define PXINTC		0x18
#define PXMSK		0x20
#define PXMSKS		0x24
#define PXMSKC		0x28
#define PXPAT1		0x30
#define PXPAT1S		0x34
#define PXPAT1C		0x38
#define PXPAT0		0x40
#define PXPAT0S		0x44
#define PXPAT0C		0x48
#define PXFLG		0x50
#define PXFLGC		0x58
#define PXPU		0x80
#define PXPUS		0x84
#define PXPUC		0x88
#define PXPD		0x90
#define PXPDS		0x94
#define PXPDC		0x98

#define PXDS0		0xa0
#define PXDS0S		0xa4
#define PXDS0C		0xa8
#define PXDS1		0xb0
#define PXDS1S		0xb4
#define PXDS1C		0xb8

#define GPIO_GROUP_OFFSET	0x1000
#define GPIO_PXPIN(n)	(GPIO_BASE + PXPIN + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXINT(n)	(GPIO_BASE + PXINT + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXINTS(n)	(GPIO_BASE + PXINTS + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXINTC(n)	(GPIO_BASE + PXINTC + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXMSK(n)	(GPIO_BASE + PXMSK + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXMSKS(n)	(GPIO_BASE + PXMSKS + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXMSKC(n)	(GPIO_BASE + PXMSKC + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXPAT1(n)	(GPIO_BASE + PXPAT1 + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXPAT1S(n)	(GPIO_BASE + PXPAT1S + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXPAT1C(n)	(GPIO_BASE + PXPAT1C + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXPAT0(n)	(GPIO_BASE + PXPAT0 + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXPAT0S(n)	(GPIO_BASE + PXPAT0S + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXPAT0C(n)	(GPIO_BASE + PXPAT0C + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXFLG(n)	(GPIO_BASE + PXFLG + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXFLGC(n)	(GPIO_BASE + PXFLGC + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXPU(n)	(GPIO_BASE + PXPU + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXPUS(n)	(GPIO_BASE + PXPUS + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXPUC(n)	(GPIO_BASE + PXPUC + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXPD(n)	(GPIO_BASE + PXPD + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXPDS(n)	(GPIO_BASE + PXPDS + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXPDC(n)	(GPIO_BASE + PXPDC + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXDS0(n)	(GPIO_BASE + PXDS0 + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXDS0S(n)	(GPIO_BASE + PXDS0S + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXDS0C(n)	(GPIO_BASE + PXDS0C + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXDS1(n)	(GPIO_BASE + PXDS1 + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXDS1S(n)	(GPIO_BASE + PXDS1S + (n) * GPIO_GROUP_OFFSET)
#define GPIO_PXDS1C(n)	(GPIO_BASE + PXDS1C + (n) * GPIO_GROUP_OFFSET)

void gpio_set_func(enum gpio_port n, enum gpio_function func, unsigned int pins);
void gpio_port_set_value(int port, int pin, int value);
void gpio_port_direction_input(int port, int pin);
void gpio_port_direction_output(int port, int pin, int value);
void gpio_init(void);
void gpio_enable_pull(unsigned gpio);
void gpio_disable_pull(unsigned gpio);
void gpio_as_irq_high_level(unsigned gpio);
void gpio_as_irq_low_level(unsigned gpio);
void gpio_as_irq_rise_edge(unsigned gpio);
void gpio_as_irq_fall_edge(unsigned gpio);
void gpio_ack_irq(unsigned gpio);
int gpio_clear_flag(unsigned gpio);
int gpio_get_flag(unsigned int gpio);
void gpio_set_driver_strength(enum gpio_port gpio, int value, unsigned int pins);

#endif /* __X3000_GPIO_H__ */
