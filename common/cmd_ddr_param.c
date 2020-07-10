/*
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <common.h>
#include <command.h>
#include <asm/errno.h>
#include <asm/arch/clk.h>


#include <ddr/ddrc_x2000.h>


struct ddr_parameters {
	ddrc_timing1_t *timing1;
	ddrc_timing2_t *timing2;
	ddrc_timing3_t *timing3;
	ddrc_timing4_t *timing4;
	ddrc_timing5_t *timing5;
};


static unsigned long long clk2ps(unsigned int clk)
{
	unsigned long long ddr_rate;
	ddr_rate = (unsigned long long)clk_get_rate(DDR);
	return 1000*1000*1000*1000LL / ddr_rate * clk;
}
#define ddr_timing_print(c,x) \
	printf("t"#x":			%d(tck)		%lld(ps)\n",ddr_p->timing##c->b.t##x, clk2ps(ddr_p->timing##c->b.t##x))

#define ddr_timing_print_multi(c,x, m) \
	printf("t"#x":			%dx%d(tck)	%lld(ps)\n",ddr_p->timing##c->b.t##x, m, clk2ps(ddr_p->timing##c->b.t##x * m))

static int print_ddr_param(struct ddr_parameters *ddr_p)
{
	printf("=============================DDR_PARAMS START=======================\n");
	printf("========ddr_rate: %lld Hz\n", (unsigned long long)clk_get_rate(DDR));
	printf("timing 1\n");

	ddr_timing_print(1, WL);
	ddr_timing_print(1, WR);
	ddr_timing_print(1, WTR);
	ddr_timing_print(1, WDLAT);

	printf("timing 2\n");
	ddr_timing_print(2, RL);
	ddr_timing_print(2, RTP);
	ddr_timing_print(2, RTW);
	ddr_timing_print(2, RDLAT);

	printf("timing 3\n");
	ddr_timing_print(3, RP);
	ddr_timing_print(3, CCD);
	ddr_timing_print(3, RCD);
	ddr_timing_print(3, EXTRW);

	printf("timing 4\n");
	ddr_timing_print(4, RRD);
	ddr_timing_print(4, RAS);
	ddr_timing_print(4, RC);
	ddr_timing_print(4, FAW);

	printf("timing 5\n");
	ddr_timing_print(5, CKE);
	ddr_timing_print(5, XP);
	ddr_timing_print(5, CKSRE);
	ddr_timing_print(5, CKESR);
	ddr_timing_print_multi(5, XS, 4);	//*4

	printf("=============================DDR_PARAMS  END =======================\n");
}

static int do_ddr_param_read(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char *str_env;
	struct ddr_parameters ddr_param;

	printf("%s %d %d\n",__FILE__,__LINE__,argc);
	if (argc > 1)
		return CMD_RET_USAGE;

	ddr_param.timing1 = (ddrc_timing1_t *)(0xb34f0040);
	ddr_param.timing2 = (ddrc_timing2_t *)(0xb34f0048);
	ddr_param.timing3 = (ddrc_timing3_t *)(0xb34f0050);
	ddr_param.timing4 = (ddrc_timing4_t *)(0xb34f0058);
	ddr_param.timing5 = (ddrc_timing5_t *)(0xb34f0060);

	print_ddr_param(&ddr_param);

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(ddrc_timings, 1, 1, do_ddr_param_read,
	"ddrc_timing for X2000/M300",
	"no param\n"
);
