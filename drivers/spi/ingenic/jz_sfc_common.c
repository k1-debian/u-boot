/*
 * SFC controller for SPI protocol, use FIFO and DMA;
 *
 * Copyright (c) 2015 Ingenic
 * Author: <xiaoyang.fu@ingenic.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <asm/arch/clk.h>
#include <asm/gpio.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <asm/arch/base.h>
#include <asm/io.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/err.h>
#include <common.h>

#include <asm/arch/sfc.h>


//#define	SFC_NOR_DEBUG



static void sfc_writel(struct sfc *sfc, unsigned short offset, u32 value)
{
	writel(value, SFC_BASE + offset);
}

static unsigned int sfc_readl(struct sfc *sfc, unsigned short offset)
{
	return readl(SFC_BASE + offset);
}

#ifdef SFC_NOR_DEBUG
void dump_sfc_reg(struct sfc *sfc)
{
	int i = 0;
	printf("SFC_GLB			:%08x\n", sfc_readl(sfc, SFC_GLB ));
	printf("SFC_DEV_CONF	:%08x\n", sfc_readl(sfc, SFC_DEV_CONF ));
	printf("SFC_DEV_STA_EXP	:%08x\n", sfc_readl(sfc, SFC_DEV_STA_EXP));
	printf("SFC_DEV_STA_RT	:%08x\n", sfc_readl(sfc, SFC_DEV_STA_RT ));
	printf("SFC_DEV_STA_MSK	:%08x\n", sfc_readl(sfc, SFC_DEV_STA_MSK ));
	printf("SFC_TRAN_LEN		:%08x\n", sfc_readl(sfc, SFC_TRAN_LEN ));

	for(i = 0; i < 6; i++)
		printf("SFC_TRAN_CONF(%d)	:%08x\n", i,sfc_readl(sfc, SFC_TRAN_CONF(i)));

	for(i = 0; i < 6; i++)
		printf("SFC_DEV_ADDR(%d)	:%08x\n", i,sfc_readl(sfc, SFC_DEV_ADDR(i)));

	printf("SFC_MEM_ADDR :%08x\n", sfc_readl(sfc, SFC_MEM_ADDR ));
	printf("SFC_TRIG	 :%08x\n", sfc_readl(sfc, SFC_TRIG));
	printf("SFC_SR		 :%08x\n", sfc_readl(sfc, SFC_SR));
	printf("SFC_SCR		 :%08x\n", sfc_readl(sfc, SFC_SCR));
	printf("SFC_INTC	 :%08x\n", sfc_readl(sfc, SFC_INTC));
	printf("SFC_FSM		 :%08x\n", sfc_readl(sfc, SFC_FSM ));
	printf("SFC_CGE		 :%08x\n", sfc_readl(sfc, SFC_CGE ));
//	printf("SFC_RM_DR 	 :%08x\n", sfc_readl(spi, SFC_RM_DR));
}

static void dump_data(unsigned char *buf,size_t len)
{
	int i;
	for(i = 0;i<len;i++){
		if(!(i % 16)){
			printk("\n");
			printk("%08x:",i);
		}
		printk("%02x ",buf[i]);
	}
}
#endif

void sfc_init(struct sfc *sfc)
{
	int n;
	for(n = 0; n < N_MAX; n++) {
		sfc_writel(sfc, SFC_TRAN_CONF(n), 0);
		sfc_writel(sfc, SFC_DEV_ADDR(n), 0);
		sfc_writel(sfc, SFC_DEV_ADDR_PLUS(n), 0);
	}

	sfc_writel(sfc, SFC_DEV_CONF, 0);
	sfc_writel(sfc, SFC_DEV_STA_EXP, 0);
	sfc_writel(sfc, SFC_DEV_STA_MSK, 0);
	sfc_writel(sfc, SFC_TRAN_LEN, 0);
	sfc_writel(sfc, SFC_MEM_ADDR, 0);
	sfc_writel(sfc, SFC_TRIG, 0);
	sfc_writel(sfc, SFC_SCR, 0);
	sfc_writel(sfc, SFC_INTC, 0);
	sfc_writel(sfc, SFC_CGE, 0);
	sfc_writel(sfc, SFC_RM_DR, 0);
}

void sfc_stop(struct sfc*sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRIG);
	tmp |= TRIG_STOP;
	sfc_writel(sfc, SFC_TRIG, tmp);
}

void sfc_start(struct sfc *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRIG);
	tmp |= TRIG_START;
	sfc_writel(sfc, SFC_TRIG, tmp);
}

void sfc_flush_fifo(struct sfc *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRIG);
	tmp |= TRIG_FLUSH;
	sfc_writel(sfc, SFC_TRIG, tmp);
}

void sfc_ce_invalid_value(struct sfc *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp &= ~DEV_CONF_CEDL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp |= DEV_CONF_CEDL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	}
}

void sfc_hold_invalid_value(struct sfc *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp &= ~DEV_CONF_HOLDDL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp |= DEV_CONF_HOLDDL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	}
}

void sfc_wp_invalid_value(struct sfc *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp &= ~DEV_CONF_WPDL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp |= DEV_CONF_WPDL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	}
}

void sfc_clear_end_intc(struct sfc *sfc)
{
	int tmp = 0;
	tmp = CLR_END;
	sfc_writel(sfc, SFC_SCR, tmp);
}

void sfc_clear_treq_intc(struct sfc *sfc)
{
	int tmp = 0;
	tmp = CLR_TREQ;
	sfc_writel(sfc, SFC_SCR, tmp);
}

void sfc_clear_rreq_intc(struct sfc *sfc)
{
	int tmp = 0;
	tmp = CLR_RREQ;
	sfc_writel(sfc, SFC_SCR, tmp);
}

void sfc_clear_over_intc(struct sfc *sfc)
{
	int tmp = 0;
	tmp = CLR_OVER;
	sfc_writel(sfc, SFC_SCR, tmp);
}

void sfc_clear_under_intc(struct sfc *sfc)
{
	int tmp = 0;
	tmp = CLR_UNDER;
	sfc_writel(sfc, SFC_SCR, tmp);
}

void sfc_clear_all_intc(struct sfc *sfc)
{
	sfc_writel(sfc, SFC_SCR, 0x1f);
}

void sfc_mask_all_intc(struct sfc *sfc)
{
	sfc_writel(sfc, SFC_INTC, 0x1f);
}

void sfc_mode(struct sfc *sfc, int channel, int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
	tmp &= ~(TRAN_CONF_TRAN_MODE_MSK << TRAN_CONF_TRAN_MODE_OFFSET);
	tmp |= (value << TRAN_CONF_TRAN_MODE_OFFSET);
	sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
}
void sfc_set_phase_num(struct sfc *sfc,int num)
{
	unsigned int tmp;

	tmp = sfc_readl(sfc, SFC_GLB);
	tmp &= ~GLB_PHASE_NUM_MSK;
	tmp |= num << GLB_PHASE_NUM_OFFSET;
	sfc_writel(sfc, SFC_GLB, tmp);
}
void sfc_clock_phase(struct sfc *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp &= ~DEV_CONF_CPHA;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp |= DEV_CONF_CPHA;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	}
}

void sfc_clock_polarity(struct sfc *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp &= ~DEV_CONF_CPOL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp |= DEV_CONF_CPOL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	}
}

void sfc_threshold(struct sfc *sfc, int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_GLB);
	tmp &= ~GLB_THRESHOLD_MSK;
	tmp |= value << GLB_THRESHOLD_OFFSET;
	sfc_writel(sfc, SFC_GLB, tmp);
}


void sfc_smp_delay(struct sfc *sfc, int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_DEV_CONF);
	tmp &= ~DEV_CONF_SMP_DELAY_MSK;
	tmp |= value << DEV_CONF_SMP_DELAY_OFFSET;
	sfc_writel(sfc, SFC_DEV_CONF, tmp);
}

void sfc_hold_delay(struct sfc *sfc, int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_DEV_CONF);
	tmp &= ~DEV_CONF_THOLD_MSK;
	tmp |= value << DEV_CONF_THOLD_OFFSET;
	sfc_writel(sfc, SFC_DEV_CONF, tmp);
}

void sfc_setup_delay(struct sfc *sfc, int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_DEV_CONF);
	tmp &= ~DEV_CONF_TSETUP_MSK;
	tmp |= value << DEV_CONF_TSETUP_OFFSET;
	sfc_writel(sfc, SFC_DEV_CONF, tmp);
}

void sfc_interval_delay(struct sfc *sfc, int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_DEV_CONF);
	tmp &= ~DEV_CONF_TSH_MSK;
	tmp |= value << DEV_CONF_TSH_OFFSET;
	sfc_writel(sfc, SFC_DEV_CONF, tmp);
}

void sfc_set_cmd_length(struct sfc *sfc, unsigned int value)
{
	if(value == 1){
		unsigned int tmp;
		tmp = sfc_readl(sfc,SFC_DEV_CONF);
		tmp &= ~TRAN_CONF_CMD_LEN;
		sfc_writel(sfc,SFC_DEV_CONF,tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc,SFC_DEV_CONF);
		tmp |= TRAN_CONF_CMD_LEN;
		sfc_writel(sfc,SFC_DEV_CONF,tmp);
	}
}

void sfc_transfer_direction(struct sfc *sfc, int value)
{
	if(value == GLB_TRAN_DIR_READ) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_GLB);
		tmp &= ~GLB_TRAN_DIR;
		sfc_writel(sfc, SFC_GLB, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_GLB);
		tmp |= GLB_TRAN_DIR;
		sfc_writel(sfc, SFC_GLB, tmp);
	}
}


int set_flash_timing(struct sfc *sfc, unsigned int t_hold, unsigned int t_setup, unsigned int t_shslrd, unsigned int t_shslwr)
{
	unsigned int c_hold;
	unsigned int c_setup;
	unsigned int t_in, c_in, val;
	unsigned long cycle;
	unsigned long long ns;
	unsigned int rate;

	rate = sfc->src_clk / 1000000;
	cycle = 1000 / rate;

	c_hold = t_hold / cycle;
	if(c_hold > 0)
		val = c_hold - 1;
	sfc_hold_delay(sfc, val);

	c_setup = t_setup / cycle;
	if(c_setup > 0)
		val = c_setup - 1;
	sfc_setup_delay(sfc, val);

	t_in = max(t_shslrd, t_shslwr);
	c_in = t_in / cycle;
	if(c_in > 0)
		val = c_in - 1;
	sfc_interval_delay(sfc, val);

	return 0;
}

void sfc_set_length(struct sfc *sfc, int value)
{
	sfc_writel(sfc, SFC_TRAN_LEN, value);
}

void sfc_transfer_mode(struct sfc *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_GLB);
		tmp &= ~GLB_OP_MODE;
		sfc_writel(sfc, SFC_GLB, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_GLB);
		tmp |= GLB_OP_MODE;
		sfc_writel(sfc, SFC_GLB, tmp);
	}
}

void sfc_read_data(struct sfc *sfc, unsigned int *value)
{
	*value = sfc_readl(sfc, SFC_RM_DR);
}

void sfc_write_data(struct sfc *sfc, const unsigned int value)
{
	sfc_writel(sfc, SFC_RM_DR, value);
}

unsigned int sfc_fifo_num(struct sfc *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_SR);
	tmp &= (0x7f << 16);
	tmp = tmp >> 16;
	return tmp;
}

static unsigned int cpu_read_rxfifo(struct sfc *sfc)
{
	int i;
	unsigned long align_len = 0;
	unsigned int fifo_num = 0;
	unsigned int data[1] = {0};
	unsigned int last_word = 0;

	align_len = ALIGN(sfc->transfer->len, 4);

	if(((align_len - sfc->transfer->cur_len) / 4) > THRESHOLD) {
		fifo_num = THRESHOLD;
		last_word = 0;
	} else {
		/* last aligned THRESHOLD data*/
		if(sfc->transfer->len % 4) {
			fifo_num = (align_len - sfc->transfer->cur_len) / 4 - 1;
			last_word = 1;
		} else {
			fifo_num = (align_len - sfc->transfer->cur_len) / 4;
			last_word = 0;
		}
	}

	for(i = 0; i < fifo_num; i++) {
		sfc_read_data(sfc, (unsigned int *)sfc->transfer->data);
		sfc->transfer->data += 4;
		sfc->transfer->cur_len += 4;
	}

	/* last word */
	if(last_word == 1) {
		sfc_read_data(sfc, data);
		memcpy((void *)sfc->transfer->data, data, sfc->transfer->len % 4);

		sfc->transfer->data += sfc->transfer->len % 4;
		sfc->transfer->cur_len += 4;
	}
	return 0;
}

static unsigned int cpu_write_txfifo(struct sfc *sfc)
{
	int i;
	unsigned long align_len = 0;
	unsigned int fifo_num = 0;


	align_len = ALIGN(sfc->transfer->len , 4);

	if (((align_len - sfc->transfer->cur_len) / 4) > THRESHOLD){
		fifo_num = THRESHOLD;
	} else {
		fifo_num = (align_len - sfc->transfer->cur_len) / 4;
	}

	for(i = 0; i < fifo_num; i++) {
		sfc_write_data(sfc, *(unsigned int *)sfc->transfer->data);
		sfc->transfer->data += 4;
		sfc->transfer->cur_len += 4;
	}

	return 0;
}


int sfc_underrun(struct sfc *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_SR);
	if(tmp & CLR_UNDER)
		return 1;
	else
		return 0;
}

int sfc_overrun(struct sfc *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_SR);
	if(tmp & CLR_OVER)
		return 1;
	else
		return 0;
}

int rxfifo_rreq(struct sfc *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_SR);
	if(tmp & CLR_RREQ)
		return 1;
	else
		return 0;
}
int txfifo_treq(struct sfc *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_SR);
	if(tmp & CLR_TREQ)
		return 1;
	else
		return 0;
}
int sfc_end(struct sfc *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_SR);
	if(tmp & CLR_END)
		return 1;
	else
		return 0;
}
unsigned int sfc_get_sta_rt(struct sfc *sfc)
{
	return sfc_readl(sfc,SFC_DEV_STA_RT);
}
unsigned int sfc_get_fsm(struct sfc *sfc)
{
	return sfc_readl(sfc,SFC_FSM);
}
void sfc_set_addr_length(struct sfc *sfc, int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
	tmp &= ~(ADDR_WIDTH_MSK);
	tmp |= (value << ADDR_WIDTH_OFFSET);
	sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
}

void sfc_cmd_enble(struct sfc *sfc, int channel, unsigned int value)
{
	if(value == ENABLE) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
		tmp |= TRAN_CONF_CMDEN;
		sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
		tmp &= ~TRAN_CONF_CMDEN;
		sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
	}
}

void sfc_data_en(struct sfc *sfc, int channel, unsigned int value)
{
	if(value == 1) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
		tmp |= TRAN_CONF_DATEEN;
		sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
		tmp &= ~TRAN_CONF_DATEEN;
		sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
	}
}

void sfc_phase_format(struct sfc *sfc, int channel, unsigned int value)
{
	if(value == 1) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
		tmp |= TRAN_CONF_FMAT;
		sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
		tmp &= ~TRAN_CONF_FMAT;
		sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
	}
}

void sfc_write_cmd(struct sfc *sfc, int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
	tmp &= ~TRAN_CONF_CMD_MSK;
	tmp |= value;
	sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
}

void sfc_dev_addr(struct sfc *sfc, int channel, unsigned int value)
{
	sfc_writel(sfc, SFC_DEV_ADDR(channel), value);
}


void sfc_dev_data_dummy_bytes(struct sfc *sfc, int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
	tmp &= ~TRAN_CONF_DMYBITS_MSK;
	tmp |= value << DMYBITS_OFFSET;
	sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
}

void sfc_dev_addr_plus(struct sfc *sfc, int channel, unsigned int value)
{
	sfc_writel(sfc, SFC_DEV_ADDR_PLUS(channel), value);
}

void sfc_dev_pollen(struct sfc *sfc, int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
	if(value == 1)
		tmp |= TRAN_CONF_POLLEN;
	else
		tmp &= ~(TRAN_CONF_POLLEN);

	sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
}

void sfc_dev_sta_exp(struct sfc *sfc, unsigned int value)
{
	sfc_writel(sfc, SFC_DEV_STA_EXP, value);
}

void sfc_dev_sta_msk(struct sfc *sfc, unsigned int value)
{
	sfc_writel(sfc, SFC_DEV_STA_MSK, value);
}

void sfc_enable_all_intc(struct sfc *sfc)
{
	sfc_writel(sfc, SFC_INTC, 0);
}

void sfc_set_mem_addr(struct sfc *sfc,unsigned int addr )
{
	sfc_writel(sfc, SFC_MEM_ADDR, addr);
}

static int sfc_sr_handle(struct sfc *sfc)
{
	int ret;
	while (1) {
		if (sfc_underrun(sfc)) {
			sfc_clear_under_intc(sfc);
			printk("sfc UNDR !\n");
			ret = -1;
			break;
		}

		if (sfc_overrun(sfc)) {
			sfc_clear_over_intc(sfc);
			printk("sfc OVER !\n");
			ret = -1;
			break;
		}

		if (rxfifo_rreq(sfc)) {
			sfc_clear_rreq_intc(sfc);
			cpu_read_rxfifo(sfc);
		}

		if (txfifo_treq(sfc)) {
			sfc_clear_treq_intc(sfc);
			cpu_write_txfifo(sfc);
		}

		if(sfc_end(sfc)){
			sfc_clear_end_intc(sfc);
			ret = 0;
			break;
		}
	}
	return ret;
}

static int sfc_start_transfer(struct sfc *sfc)
{
	int ret;
	sfc_clear_all_intc(sfc);
	sfc_mask_all_intc(sfc);
	sfc_start(sfc);

	ret = sfc_sr_handle(sfc);

	return ret;
}
static void sfc_phase_transfer(struct sfc *sfc,struct sfc_transfer *
		transfer,int channel)
{
	sfc_flush_fifo(sfc);
	sfc_set_addr_length(sfc,channel,transfer->addr_len);
	sfc_cmd_enble(sfc,channel,ENABLE);
	sfc_write_cmd(sfc,channel,transfer->cmd_info->cmd);
	sfc_dev_data_dummy_bytes(sfc,channel,transfer->data_dummy_bits);
	sfc_data_en(sfc,channel,transfer->cmd_info->dataen);
	sfc_dev_addr(sfc, channel,transfer->addr);
	sfc_dev_addr_plus(sfc,channel,transfer->addr_plus);
	sfc_mode(sfc,channel,transfer->sfc_mode);
	sfc_phase_format(sfc,channel,0);/*default 0,dummy bits is blow the addr*/

}
static void common_cmd_request_transfer(struct sfc *sfc,struct sfc_transfer *transfer,int channel)
{
	sfc_phase_transfer(sfc,transfer,channel);
	sfc_dev_sta_exp(sfc,0);
	sfc_dev_sta_msk(sfc,0);
	sfc_dev_pollen(sfc,channel,DISABLE);
}

static void poll_cmd_request_transfer(struct sfc *sfc,struct sfc_transfer *transfer,int channel)
{
	struct cmd_info *cmd = transfer->cmd_info;
	sfc_phase_transfer(sfc,transfer,channel);
	sfc_dev_sta_exp(sfc,cmd->sta_exp);
	sfc_dev_sta_msk(sfc,cmd->sta_msk);
	sfc_dev_pollen(sfc,channel,ENABLE);
}
static void sfc_glb_info_config(struct sfc *sfc,struct sfc_transfer *transfer)
{
	sfc_transfer_direction(sfc, transfer->direction);
	if((transfer->ops_mode == DMA_OPS)){
		sfc_set_length(sfc, transfer->len);
		if(transfer->direction == GLB_TRAN_DIR_READ)
			flush_cache_all();
		else
			flush_cache_all();
		sfc_set_mem_addr(sfc, virt_to_phys(transfer->data));
		sfc_transfer_mode(sfc, DMA_MODE);
	}else{
		sfc_set_length(sfc, transfer->len);
		sfc_set_mem_addr(sfc, 0);
		sfc_transfer_mode(sfc, SLAVE_MODE);
	}
}
#ifdef SFC_NOR_DEBUG
static void  dump_transfer(struct sfc_transfer *xfer,int num)
{
	printf("\n");
	printf("cmd[%d].cmd = 0x%02x\n",num,xfer->cmd_info->cmd);
	printf("cmd[%d].addr_len = %d\n",num,xfer->addr_len);
	printf("cmd[%d].dummy_byte = %d\n",num,xfer->data_dummy_bits);
	printf("cmd[%d].dataen = %d\n",num,xfer->cmd_info->dataen);
	printf("cmd[%d].sta_exp = %d\n",num,xfer->cmd_info->sta_exp);
	printf("cmd[%d].sta_msk = %d\n",num,xfer->cmd_info->sta_msk);


	printf("transfer[%d].addr = 0x%08x\n",num,xfer->addr);
	printf("transfer[%d].len = %d\n",num,xfer->len);
	printf("transfer[%d].data = 0x%p\n",num,xfer->data);
	printf("transfer[%d].direction = %d\n",num,xfer->direction);
	printf("transfer[%d].sfc_mode = %d\n",num,xfer->sfc_mode);
	printf("transfer[%d].ops_mode = %d\n",num,xfer->ops_mode);
}
#endif
int sfc_sync(struct sfc *sfc, struct sfc_message *message)
{
	struct sfc_transfer *xfer;
	int phase_num = 0,ret = 0;

	sfc_set_length(sfc, 0);
	list_for_each_entry(xfer, &message->transfers, transfer_list) {
		if(xfer->cmd_info->sta_msk == 0){
			common_cmd_request_transfer(sfc,xfer,phase_num);
		}else{
			poll_cmd_request_transfer(sfc,xfer,phase_num);
		}
		if(xfer->addr_len || xfer->len)
			sfc_glb_info_config(sfc,xfer);
		phase_num++;
		message->actual_length += xfer->len;
		if(xfer->len > 0)
			sfc->transfer = xfer;
	}
	sfc_set_phase_num(sfc,phase_num);
	ret = sfc_start_transfer(sfc);
	list_del_init(&message->transfers);
	return ret;
}


void sfc_transfer_del(struct sfc_transfer *t)
{
	list_del(&t->transfer_list);
}

void sfc_message_add_tail(struct sfc_transfer *t, struct sfc_message *m)
{
	list_add_tail(&t->transfer_list, &m->transfers);
}

void sfc_message_init(struct sfc_message *m)
{
	memset(m, 0, sizeof *m);
	INIT_LIST_HEAD(&m->transfers);
}


static int sfc_ctl_init(struct sfc *sfc)
{
	sfc_init(sfc);
	sfc_stop(sfc);

	/*set hold high*/
	sfc_hold_invalid_value(sfc, 1);
	/*set wp high*/
	sfc_wp_invalid_value(sfc, 1);

	sfc_clear_all_intc(sfc);
	sfc_mask_all_intc(sfc);

	sfc_threshold(sfc, sfc->threshold);
	/*config the sfc pin init state*/
	sfc_clock_phase(sfc, 0);
	sfc_clock_polarity(sfc, 0);
	sfc_ce_invalid_value(sfc, 1);


	sfc_transfer_mode(sfc, SLAVE_MODE);
	if(sfc->src_clk >= 100000000){
		sfc_smp_delay(sfc,DEV_CONF_HALF_CYCLE_DELAY);
	}
	return 0;
}
struct sfc *sfc_res_init()
{
	struct sfc *sfc = NULL;
	sfc = malloc(sizeof(struct sfc));
	if (!sfc) {
		printf("ERROR: %s %d kzalloc() error !\n",__func__,__LINE__);
		return ERR_PTR(-ENOMEM);
	}
	memset(sfc, 0, sizeof(struct sfc));

	sfc->src_clk = CONFIG_SFC_RATE;
	clk_set_rate(SFC, sfc->src_clk);

	sfc->threshold = THRESHOLD;

	sfc_ctl_init(sfc);

	return sfc;

}
