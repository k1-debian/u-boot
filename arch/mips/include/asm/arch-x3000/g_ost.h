#ifndef __X3000_G_OST_H__
#define __X3000_G_OST_H__

#include <asm/arch/base.h>

#define G_OSTCCR	0x00
#define G_OSTER		0x04
#define G_OSTCR		0x08
#define G_OSTCNTH	0x0c
#define G_OSTCNTL	0x10
#define G_OSTCNTB	0x14

#define G_OSTCCR_PRESCALE_1	(0 << 0)
#define G_OSTCCR_PRESCALE_4	(1 << 0)
#define G_OSTCCR_PRESCALE_16	(2 << 0)

#define G_OST_DIV	4
#define G_OSTCCR_PRESCALE	G_OSTCCR_PRESCALE_4

#endif /* __X3000_G_OST_H__ */
