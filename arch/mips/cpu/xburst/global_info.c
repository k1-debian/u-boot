#include <asm/global_info.h>

void dump_gi(struct global_info *gi)
{
	printf("global_info: 0x%x\n", gi);
	printf("extal: %x\n", gi->extal);
	printf("cpufreq: %x\n", gi->cpufreq);
	printf("ddrfreq: %x\n", gi->ddrfreq);
	printf("uart_idx: %x\n", gi->uart_idx);
	printf("baud_rate: %x\n", gi->baud_rate);
}
void dump_gp(struct global_shared_params *gp)
{
	printf("global_shared_params: 0x%x\n", gp);
	printf("version: %x\n", gp->version);
	printf("extal: %x\n", gp->extal);
	printf("cpufreq: %x\n", gp->cpufreq);
	printf("ddrfreq: %x\n", gp->ddrfreq);
	printf("uart_idx: %x\n", gp->uart_idx);
	printf("baud_rate: %x\n", gp->baud_rate);
}

void ginfo_set(struct global_info *gi, struct global_shared_params *gp)
{
	gi->extal = gp->extal;
	gi->cpufreq = gp->cpufreq;
	gi->ddrfreq = gp->ddrfreq;
	gi->uart_idx = gp->uart_idx;
	gi->baud_rate = gp->baud_rate;
}
