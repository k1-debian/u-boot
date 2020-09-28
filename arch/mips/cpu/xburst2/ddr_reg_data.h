#ifndef DDR_REG_DATA_H
#define DDR_REG_DATA_H
struct ddr_registers
{
	uint32_t ddrc_cfg;
	uint32_t ddrc_ctrl;
	uint32_t ddrc_dlmr;
	uint32_t ddrc_ddlp;
	uint32_t ddrc_mmap[2];
	uint32_t ddrc_refcnt;
	uint32_t ddrc_timing1;
	uint32_t ddrc_timing2;
	uint32_t ddrc_timing3;
	uint32_t ddrc_timing4;
	uint32_t ddrc_timing5;
	uint32_t ddrc_autosr_cnt;
	uint32_t ddrc_autosr;
	uint32_t ddrc_hregpro;
	uint32_t ddrc_pregpro;
	uint32_t ddrc_cguc0;
	uint32_t ddrc_cguc1;
	uint32_t ddrp_memcfg;
	uint32_t ddrp_cl;
	uint32_t ddrp_cwl;
	uint32_t ddr_mr0;
	uint32_t ddr_mr1;
	uint32_t ddr_mr2;
	uint32_t ddr_mr3;
	uint32_t ddr_mr10;
	uint32_t ddr_mr11;
	uint32_t ddr_mr63;
	uint32_t ddr_chip0_size;
	uint32_t ddr_chip1_size;
	unsigned int remap_array[5];
};
extern struct ddr_registers *g_ddr_param;

#if 0
static unsigned int get_refcnt_value(int div)
{
	switch(div) {
		case 0: return 0xe480000;
		case 1: return 0x7230000;
		case 2: return 0x5170000;
		defalut :
			printf("not support \n");
	}
}
#endif

#endif /* DDR_REG_DATA_H */
