/*
 * SPL params fixer for X3000 preloading.
 *
 * The BootROM consumes this parameter block before jumping into SPL. Keep the
 * clock sequence aligned with the X3000 bring-up reference: program APLL and safe
 * dividers first, keep boot-media clocks divided down, then switch SCLK_A and
 * AHB selectors before enabling DDR clock.
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <asm/arch/cpm.h>

#ifndef BITS_H2L
#define BITS_H2L(msb, lsb)	((0xffffffffU >> (32 - ((msb) - (lsb) + 1))) << (lsb))
#endif

#ifndef JZ_EXCLK
#define JZ_EXCLK		CONFIG_SYS_EXTAL
#endif

/* APLL Control Register (CPAPCR) */
#define CPAPCR_PLLFD_LSB	20
#define CPAPCR_PLLFD_MASK	BITS_H2L(31, CPAPCR_PLLFD_LSB)

#define CPAPCR_PLLRD_LSB	14
#define CPAPCR_PLLRD_MASK	BITS_H2L(19, CPAPCR_PLLRD_LSB)

#define CPAPCR_PLLOD1_LSB	11
#define CPAPCR_PLLOD1_MASK	BITS_H2L(13, CPAPCR_PLLOD1_LSB)

#define CPAPCR_PLLOD0_LSB	8
#define CPAPCR_PLLOD0_MASK	BITS_H2L(10, CPAPCR_PLLOD0_LSB)

#define CPAPCR_PLL_ON		(1U << 3)
#define CPAPCR_PLL_LOCK		(1U << 2)
#define CPAPCR_PLL_EN		(1U << 0)

#define X3000_CPCSR_AHB2_MUX_STABLE	(1U << 30)
#define X3000_CPCSR_AHB1_MUX_STABLE	(1U << 29)
#define X3000_CPCSR_AHB0_MUX_STABLE	(1U << 28)
#define X3000_CPCSR_SRC_MUX_STABLE	(1U << 27)
#define X3000_CPCSR_CPU_MUX_STABLE	(1U << 24)

#define X3000_CPCSR_H2DIV_BUSY		(1U << 6)
#define X3000_CPCSR_H1DIV_BUSY		(1U << 5)
#define X3000_CPCSR_H0DIV_BUSY		(1U << 4)
#define X3000_CPCSR_CDIV_BUSY		(1U << 0)

#define SEL_SCLKA		2
#define SEL_CPU			1
#define SEL_H0			1
#define SEL_H1			1
#define SEL_H2			1
#define DIV_PCLK		8
#define DIV_H2			4
#define DIV_H1			4
#define DIV_H0			4
#define DIV_L2			2
#define DIV_CPU			1

#define CPCCR_CFG		(((SEL_SCLKA & 3) << 30)		\
				 | ((SEL_CPU & 3) << 24)		\
				 | (((DIV_L2 - 1) & 0xf) << 4)		\
				 | (((DIV_CPU - 1) & 0xf) << 0))

#define CPCCR1_CFG		(((SEL_H2 & 3) << 28)			\
				 | ((SEL_H1 & 3) << 26)			\
				 | ((SEL_H0 & 3) << 24)			\
				 | (1U << 18) | (1U << 17)		\
				 | (1U << 16)				\
				 | (((DIV_PCLK - 1) & 0xf) << 12)	\
				 | (((DIV_H2 - 1) & 0xf) << 8)		\
				 | (((DIV_H1 - 1) & 0xf) << 4)		\
				 | (((DIV_H0 - 1) & 0xf) << 0))

#define CONFIG_BOOTROM_PLLFREQ		624000000
#define CONFIG_BOOTROM_CPMCPCCR		CPCCR_CFG

struct desc {
	unsigned set_addr:16;
	unsigned poll_addr:16;
	unsigned value:32;
	unsigned poll_h_mask:32;
	unsigned poll_l_mask:32;
};

typedef union reg_cpccr {
	uint32_t d32;
	struct {
		unsigned CDIV:4;
		unsigned L2CDIV:4;
		unsigned LEPDIV:4;
		unsigned reserved0:4;
		unsigned CE_CPU:1;
		unsigned reserved1:1;
		unsigned CE_LEP:1;
		unsigned GATE_SCLKA:1;
		unsigned reserved2:4;
		unsigned SEL_CPLL:2;
		unsigned reserved3:2;
		unsigned SEL_LEPPLL:2;
		unsigned SEL_SRC:2;
	} b;
} reg_cpccr_t;

typedef union nand_timing {
	uint32_t nand_timing[4];
	struct {
		unsigned set_rw:8;
		unsigned wait_rw:8;
		unsigned hold_rw:8;
		unsigned set_cs:8;
		unsigned wait_cs:8;
		unsigned trr:8;
		unsigned tedo:8;
		unsigned trpre:8;
		unsigned twpre:8;
		unsigned tds:8;
		unsigned tdh:8;
		unsigned twpst:8;
		unsigned tdqsre:8;
		unsigned trhw:8;
		unsigned t1:8;
		unsigned t2:8;
	} b;
} nand_timing_t;

struct params {
	unsigned int id;
	unsigned int length;
	unsigned int pll_freq;
	reg_cpccr_t cpccr;
	nand_timing_t nand_timing;
	struct desc cpm_desc[0];
};

/* APLL: 24MHz * 52 / (1 * 2 * 1) = 624MHz */
#define XPLL_M		52
#define XPLL_N		1
#define XPLL_OD1	2
#define XPLL_OD0	1
#define APLL_FOUT	(JZ_EXCLK * XPLL_M / (XPLL_N * XPLL_OD1 * XPLL_OD0))
#define APLL_VAL	((XPLL_M << CPAPCR_PLLFD_LSB) |		\
			 (XPLL_N << CPAPCR_PLLRD_LSB) |		\
			 (XPLL_OD1 << CPAPCR_PLLOD1_LSB) |	\
			 (XPLL_OD0 << CPAPCR_PLLOD0_LSB) |	\
			 (1U << 7) | (1U << 6) | CPAPCR_PLL_EN)
#define APLL_POLL	(CPAPCR_PLL_LOCK | CPAPCR_PLL_ON)

/*
 * X3000 splits the main clock tree into CPCCR and CPCCR1. Program divider
 * fields while muxes stay on the reset-safe source, then switch muxes.
 */
#define CPCCR_FRQ_VAL	(((1U << 30) | (1U << 24)) |	\
			 (1U << 16) | (1U << 4) | (0U << 0))
#define CPCCR_FRQ_POLL	X3000_CPCSR_CDIV_BUSY

#define CPCCR_SEL_VAL	(((2U << 30) | (1U << 24)) |	\
			 (1U << 16) | (1U << 4) | (0U << 0))
#define CPCCR_SEL_POLL	(X3000_CPCSR_SRC_MUX_STABLE | X3000_CPCSR_CPU_MUX_STABLE)

#define CPCCR1_FRQ_VAL	CPCCR1_CFG
#define CPCCR1_FRQ_POLL	(X3000_CPCSR_H2DIV_BUSY | X3000_CPCSR_H1DIV_BUSY | \
			 X3000_CPCSR_H0DIV_BUSY)

#define CPCCR1_SEL_VAL	CPCCR1_FRQ_VAL
#define CPCCR1_SEL_POLL	(X3000_CPCSR_AHB2_MUX_STABLE | \
			 X3000_CPCSR_AHB1_MUX_STABLE | \
			 X3000_CPCSR_AHB0_MUX_STABLE)

/* Select SCLK_A and divide down before SCLK_A switches to APLL. */
#define CDR_BUSY		(1U << 28)
#define DDR_VAL			((1U << 30) | (1U << 29) | (1U << 0))
#define MSC0_VAL		((0U << 30) | (1U << 29) | (3U << 0) | \
				 (1U << 20) | (1U << 15))
#define MSC1_VAL		((0U << 30) | (1U << 29) | (4U << 0) | \
				 (1U << 20) | (1U << 15))
#define MSC2_VAL		MSC1_VAL
#define SFC_VAL			((0U << 30) | (1U << 29) | (7U << 0))
#define SADC0_VAL		((3U << 30) | (1U << 29) | (1U << 20))
#define SADC1_VAL		SADC0_VAL

static const struct desc descriptors[] = {
	{CPM_CPAPCR,		CPM_CPAPCR,	APLL_VAL,	APLL_POLL,	0},
	{CPM_CPCCR,		CPM_CPCSR,	CPCCR_FRQ_VAL,	0,		CPCCR_FRQ_POLL},
	{CPM_CPCCR1,		CPM_CPCSR,	CPCCR1_FRQ_VAL,	0,		CPCCR1_FRQ_POLL},
#if defined(CONFIG_SPL_MMC_SUPPORT) || defined(CONFIG_SPL_JZMMC_SUPPORT)
#ifdef CONFIG_JZ_MMC_MSC0
	{CPM_MSC0CDR,		CPM_MSC0CDR,	MSC0_VAL,	0,		CDR_BUSY},
#endif
#ifdef CONFIG_JZ_MMC_MSC1
	{CPM_MSC1CDR,		CPM_MSC1CDR,	MSC1_VAL,	0,		CDR_BUSY},
#endif
#ifdef CONFIG_JZ_MMC_MSC2
	{CPM_MSC2CDR,		CPM_MSC2CDR,	MSC2_VAL,	0,		CDR_BUSY},
#endif
#endif
#if defined(CONFIG_SPL_SFC_NOR) || defined(CONFIG_SPL_SFC_NAND)
	{CPM_SFCCDR,		CPM_SFCCDR,	SFC_VAL,	0,		CDR_BUSY},
#endif
	{CPM_SADCCDR,		CPM_SADCCDR,	SADC0_VAL,	0,		CDR_BUSY},
	{CPM_SADC1CDR,		CPM_SADC1CDR,	SADC1_VAL,	0,		CDR_BUSY},
	{CPM_CPCCR,		CPM_CPCSR,	CPCCR_SEL_VAL,	CPCCR_SEL_POLL,	0},
	{CPM_CPCCR1,		CPM_CPCSR,	CPCCR1_SEL_VAL,	CPCCR1_SEL_POLL,0},
	{CPM_DDRCDR,		CPM_DDRCDR,	DDR_VAL,	0,		CDR_BUSY},
	{0xffff,		0xffff,		0,		0,		0},
};

static void fill_nand_timing(nand_timing_t *timing)
{
	timing->b.set_rw = 3;
	timing->b.wait_rw = 14;
	timing->b.hold_rw = 6;
	timing->b.set_cs = 20;
	timing->b.wait_cs = 6;
	timing->b.trr = 12;
	timing->b.tedo = 15;
	timing->b.trpre = 0;
	timing->b.twpre = 0;
	timing->b.tds = 0;
	timing->b.tdh = 0;
	timing->b.twpst = 0;
	timing->b.tdqsre = 0;
	timing->b.trhw = 30;
	timing->b.t1 = 0;
	timing->b.t2 = 0;
}

static void dump_params(struct params *p)
{
	int i;

	printf("SPL Params Fixer X3000:\n");
	printf("id:\t\t0x%08X (%c%c%c%c)\n", p->id,
	       ((char *)(&p->id))[0], ((char *)(&p->id))[1],
	       ((char *)(&p->id))[2], ((char *)(&p->id))[3]);
	printf("length:\t\t%u\n", p->length);
	printf("pll_freq:\t%u\n", p->pll_freq);
	printf("CPM_CPCCR:\t0x%08X\n", p->cpccr.d32);
	printf("APLL_FOUT:\t%u\n", APLL_FOUT);

	for (i = 0; i < 4; i++)
		printf("nand_timing[%d]:\t0x%08X\n", i, p->nand_timing.nand_timing[i]);

	printf("descriptors:\n");

	for (i = 0; ; i++) {
		struct desc *desc = &p->cpm_desc[i];

		if ((desc->set_addr == 0xffff) && (desc->poll_addr == 0xffff))
			break;

		printf("NO.%d:\n", i);
		printf("\tsaddr = 0x%04X\n", desc->set_addr);
		printf("\tpaddr = 0x%04X\n", desc->poll_addr);
		printf("\tvalue = 0x%08X\n", desc->value);
		printf("\tpoll_h_mask = 0x%08X\n", desc->poll_h_mask);
		printf("\tpoll_l_mask = 0x%08X\n", desc->poll_l_mask);
	}
}

int main(int argc, char *argv[])
{
	int fd, i, offset, params_length;
	char *spl_path, *fix_file;
	unsigned int spl_length = 0;
	struct params *params;
	char valid_id[4] = {'I', 'N', 'G', 'E'};
	struct desc *desc;

	if (argc != 5) {
		printf("Usage: %s fix_file spl_path offset params_length\n", argv[0]);
		return 1;
	}

	fix_file = argv[1];
	spl_path = argv[2];
	offset = atoi(argv[3]);
	params_length = atoi(argv[4]);

	fd = open(spl_path, O_RDONLY);
	if (fd < 0) {
		perror("open spl");
		return 1;
	}

	spl_length = lseek(fd, 0, SEEK_END);
	close(fd);

	params = calloc(1, params_length);
	if (!params)
		return 1;

	memcpy(&params->id, valid_id, 4);
	params->length = (spl_length & 0x1ff) == 0
		? spl_length
		: (spl_length & ~0x1ff) + 0x200;
	params->pll_freq = CONFIG_BOOTROM_PLLFREQ;
	params->cpccr.d32 = CONFIG_BOOTROM_CPMCPCCR;
	fill_nand_timing(&params->nand_timing);
	desc = params->cpm_desc;

	for (i = 0; i < sizeof(descriptors) / sizeof(descriptors[0]); i++)
		desc[i] = descriptors[i];

	dump_params(params);

	fd = open(fix_file, O_RDWR);
	if (fd < 0) {
		perror("open fix_file");
		free(params);
		return 1;
	}

	i = lseek(fd, offset, SEEK_SET);
	if (i != offset) {
		perror("lseek fix_file");
		close(fd);
		free(params);
		return 1;
	}

	if (write(fd, params, params_length) != params_length) {
		perror("write fix_file");
		close(fd);
		free(params);
		return 1;
	}

	close(fd);
	free(params);

	printf("fix %s for %s offset %d params %d spl %u\n",
	       fix_file, spl_path, offset, params_length, spl_length);

	return 0;
}
