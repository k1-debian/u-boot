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

#include <config.h>
#include <common.h>
#include <spi.h>
#include <spi_flash.h>
#include <malloc.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>
#include <asm/arch/spi.h>
#include <asm/arch/clk.h>
#include <asm/arch/base.h>
#include <malloc.h>
#include <errno.h>
#include <div64.h>

#include "sfc.h"
#include "../../spi/jz_spi.h"
#include "../../spi/jz_sf_internal.h"

unsigned int sfc_clk_rate = 0;
#define DEBUG 1
#define tCHSH   5       //hold
#define tSLCH   5       //setup
#define tSHSL_RD        20      //interval
#define tSHSL_WR        30

/*
#define GET_PHYADDR(a)  \
({						\
	unsigned int v;        \
	if (unlikely((unsigned int)(a) & 0x40000000)) {    \
	v = page_to_phys(vmalloc_to_page((const void *)(a))) | ((unsigned int)(a) & ~PAGE_MASK); \
	} else     \
	v = ((unsigned int)(a) & 0x1fffffff);                   \
	v;                                             \
 })*/
void sfc_writel(u32 value,unsigned short offset)
{
	writel(value, SFC_BASE + offset);
}

unsigned int sfc_readl(unsigned short offset)
{
	return readl(SFC_BASE + offset);
}

#ifdef DEBUG
void dump_sfc_reg()
{
	int i = 0;
	printf("SFC_GLB			:%08x\n", sfc_readl( SFC_GLB ));
	printf("SFC_DEV_CONF	:%08x\n", sfc_readl( SFC_DEV_CONF ));
	printf("SFC_DEV_STA_EXP	:%08x\n", sfc_readl(SFC_DEV_STA_EXP));
	printf("SFC_DEV_STA_RT	:%08x\n", sfc_readl(SFC_DEV_STA_RT ));
	printf("SFC_DEV_STA_MSK	:%08x\n", sfc_readl(SFC_DEV_STA_MSK ));
	printf("SFC_TRAN_LEN		:%08x\n", sfc_readl(SFC_TRAN_LEN ));

	for(i = 0; i < 6; i++)
		printf("SFC_TRAN_CONF(%d)	:%08x\n", i,sfc_readl(SFC_TRAN_CONF(i)));

	for(i = 0; i < 6; i++)
		printf("SFC_DEV_ADDR(%d)	:%08x\n", i,sfc_readl(SFC_DEV_ADDR(i)));

	printf("SFC_MEM_ADDR :%08x\n", sfc_readl(SFC_MEM_ADDR ));
	printf("SFC_TRIG	 :%08x\n", sfc_readl(SFC_TRIG));
	printf("SFC_SR		 :%08x\n", sfc_readl(SFC_SR));
	printf("SFC_SCR		 :%08x\n", sfc_readl(SFC_SCR));
	printf("SFC_INTC	 :%08x\n", sfc_readl(SFC_INTC));
	printf("SFC_FSM		 :%08x\n", sfc_readl(SFC_FSM ));
	printf("SFC_CGE		 :%08x\n", sfc_readl(SFC_CGE ));
//	printf("SFC_RM_DR 	 :%08x\n", sfc_readl(SFC_RM_DR));
}
#endif

void sfc_init()
{
	int n;
	for(n = 0; n < N_MAX; n++) {
		sfc_writel(0,SFC_TRAN_CONF(n));
		sfc_writel(0,SFC_DEV_ADDR(n));
		sfc_writel(0,SFC_DEV_ADDR_PLUS(n));
	}

	sfc_writel(0,SFC_DEV_CONF);
	sfc_writel(0,SFC_DEV_STA_EXP);
	sfc_writel(0,SFC_DEV_STA_MSK);
	sfc_writel(0,SFC_TRAN_LEN);
	sfc_writel(0,SFC_MEM_ADDR);
	sfc_writel(0,SFC_TRIG);
	sfc_writel(0,SFC_SCR);
	sfc_writel(0,SFC_INTC);
	sfc_writel(0,SFC_CGE);
	sfc_writel(0,SFC_RM_DR);
}

void sfc_stop()
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_TRIG);
	tmp |= TRIG_STOP;
	sfc_writel(tmp,SFC_TRIG);
}

void sfc_start()
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_TRIG);
	tmp |= TRIG_START;
	sfc_writel(tmp,SFC_TRIG);
}

void sfc_flush_fifo()
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_TRIG);
	tmp |= TRIG_FLUSH;
	sfc_writel(tmp ,SFC_TRIG);
}

void sfc_ce_invalid_value(int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(SFC_DEV_CONF);
		tmp &= ~DEV_CONF_CEDL;
		sfc_writel(tmp ,SFC_DEV_CONF);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(SFC_DEV_CONF);
		tmp |= DEV_CONF_CEDL;
		sfc_writel(tmp, SFC_DEV_CONF);
	}
}

void sfc_hold_invalid_value(int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(SFC_DEV_CONF);
		tmp &= ~DEV_CONF_HOLDDL;
		sfc_writel(tmp, SFC_DEV_CONF);
	} else {
		unsigned int tmp;
		tmp = sfc_readl( SFC_DEV_CONF);
		tmp |= DEV_CONF_HOLDDL;
		sfc_writel(tmp, SFC_DEV_CONF);
	}
}

void sfc_wp_invalid_value(int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(SFC_DEV_CONF);
		tmp &= ~DEV_CONF_WPDL;
		sfc_writel(tmp, SFC_DEV_CONF);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(SFC_DEV_CONF);
		tmp |= DEV_CONF_WPDL;
		sfc_writel(tmp, SFC_DEV_CONF);
	}
}

void sfc_clear_end_intc()
{
	int tmp = 0;
	tmp = sfc_readl(SFC_SCR);
	tmp |= CLR_END;
	sfc_writel(tmp, SFC_SCR);
	tmp = sfc_readl(SFC_SCR);
}

void sfc_clear_treq_intc()
{
	int tmp = 0;
	tmp = sfc_readl(SFC_SCR);
	tmp |= CLR_TREQ;
	sfc_writel(tmp ,SFC_SCR);
}

void sfc_clear_rreq_intc()
{
	int tmp = 0;
	tmp = sfc_readl(SFC_SCR);
	tmp |= CLR_RREQ;
	sfc_writel(tmp, SFC_SCR);
}

void sfc_clear_over_intc()
{
	int tmp = 0;
	tmp = sfc_readl(SFC_SCR);
	tmp |= CLR_OVER;
	sfc_writel(tmp, SFC_SCR);
}

void sfc_clear_under_intc()
{
	int tmp = 0;
	tmp = sfc_readl(SFC_SCR);
	tmp |= CLR_UNDER;
	sfc_writel(tmp, SFC_SCR);
}

void sfc_mode(int channel, int value)
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~(TRAN_CONF_TRAN_MODE_MSK << TRAN_CONF_TRAN_MODE_OFFSET);
	tmp |= (value << TRAN_CONF_TRAN_MODE_OFFSET);
	sfc_writel(tmp, SFC_TRAN_CONF(channel));
}

void sfc_set_phase_num(int num)
{
	unsigned int tmp;

	tmp = sfc_readl(SFC_GLB);
	tmp &= ~GLB_PHASE_NUM_MSK;
	tmp |= num << GLB_PHASE_NUM_OFFSET;
	sfc_writel(tmp, SFC_GLB);
}

void sfc_clock_phase(int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(SFC_DEV_CONF);
		tmp &= ~DEV_CONF_CPHA;
		sfc_writel(tmp, SFC_DEV_CONF);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(SFC_DEV_CONF);
		tmp |= DEV_CONF_CPHA;
		sfc_writel(tmp, SFC_DEV_CONF);
	}
}

void sfc_clock_polarity(int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(SFC_DEV_CONF);
		tmp &= ~DEV_CONF_CPOL;
		sfc_writel(tmp, SFC_DEV_CONF);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(SFC_DEV_CONF);
		tmp |= DEV_CONF_CPOL;
		sfc_writel(tmp, SFC_DEV_CONF);
	}
}

void sfc_threshold(int value)
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_GLB);
	tmp &= ~GLB_THRESHOLD_MSK;
	tmp |= value << GLB_THRESHOLD_OFFSET;
	sfc_writel(tmp, SFC_GLB);
}


void sfc_smp_delay(int value)
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_DEV_CONF);
	tmp &= ~DEV_CONF_SMP_DELAY_MSK;
	tmp |= value << DEV_CONF_SMP_DELAY_OFFSET;
	sfc_writel(tmp, SFC_DEV_CONF);
}

void sfc_hold_delay(int value)
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_DEV_CONF);
	tmp &= ~DEV_CONF_THOLD_MSK;
	tmp |= value << DEV_CONF_THOLD_OFFSET;
	sfc_writel(tmp, SFC_DEV_CONF);
}

void sfc_setup_delay(int value)
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_DEV_CONF);
	tmp &= ~DEV_CONF_TSETUP_MSK;
	tmp |= value << DEV_CONF_TSETUP_OFFSET;
	sfc_writel(tmp, SFC_DEV_CONF);
}

void sfc_interval_delay(int value)
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_DEV_CONF);
	tmp &= ~DEV_CONF_TSH_MSK;
	tmp |= value << DEV_CONF_TSH_OFFSET;
	sfc_writel(tmp, SFC_DEV_CONF);
}

void sfc_set_cmd_length(unsigned int value)
{
	if(value == 1){
		unsigned int tmp;
		tmp = sfc_readl(SFC_DEV_CONF);
		tmp &= ~TRAN_CONF_CMD_LEN;
		sfc_writel(tmp, SFC_DEV_CONF);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(SFC_DEV_CONF);
		tmp |= TRAN_CONF_CMD_LEN;
		sfc_writel(tmp, SFC_DEV_CONF);
	}
}

void sfc_transfer_direction(int value)
{
	if(value == GLB_TRAN_DIR_READ) {
		unsigned int tmp;
		tmp = sfc_readl( SFC_GLB);
		tmp &= ~GLB_TRAN_DIR;
		sfc_writel(tmp, SFC_GLB);
	} else {
		unsigned int tmp;
		tmp = sfc_readl( SFC_GLB);
		tmp |= GLB_TRAN_DIR;
		sfc_writel(tmp, SFC_GLB);
	}
}


int set_flash_timing(unsigned int t_hold, unsigned int t_setup, unsigned int t_shslrd, unsigned int t_shslwr)
{
	unsigned int c_hold;
	unsigned int c_setup;
	unsigned int t_in, c_in, val;
	unsigned long cycle;
	unsigned long long ns;
	unsigned int src_rate;

	src_rate = sfc_clk_rate;
	ns = 1000000000ULL;
	do_div(ns, src_rate);
	cycle = ns;

	c_hold = t_hold / cycle;
	if(c_hold > 0)
		val = c_hold - 1;
	sfc_hold_delay( val);

	c_setup = t_setup / cycle;
	if(c_setup > 0)
		val = c_setup - 1;
	sfc_setup_delay( val);

	t_in = max(t_shslrd, t_shslwr);
	c_in = t_in / cycle;
	if(c_in > 0)
		val = c_in - 1;
	sfc_interval_delay(val);

	return 0;
}

void sfc_set_length(int value)
{
	sfc_writel(value, SFC_TRAN_LEN);
}

void sfc_transfer_mode(int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(SFC_GLB);
		tmp &= ~GLB_OP_MODE;
		sfc_writel(tmp, SFC_GLB);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(SFC_GLB);
		tmp |= GLB_OP_MODE;
		sfc_writel(tmp, SFC_GLB);
	}
}

unsigned int sfc_fifo_num()
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_SR);
	tmp &= (0x7f << 16);
	tmp = tmp >> 16;
	return tmp;
}

int sfc_clk_set()
{
#ifndef CONFIG_BURNER
	sfc_clk_rate = 150000000;
	clk_set_rate(SFC, sfc_clk_rate);
#else
	if(sfc_clk_rate !=0 )
		clk_set_rate(SFC, sfc_clk_rate);
	else{
	        printf("this will be an error that the sfc rate is 0\n");
	        sfc_clk_rate = 70000000;
	        clk_set_rate(SFC, sfc_clk_rate);
	}
#endif
}

static int sfc_read_data(unsigned int *data, unsigned int length)
{
	unsigned int tmp_len = 0;
	unsigned int fifo_num = 0;
	unsigned int i;
	unsigned int reg_tmp = 0;
	unsigned int  len = (length + 3) / 4 ;
	unsigned int time_out = 1000;
	unsigned int align_len = ALIGN(length, 4);

	while(1){
		reg_tmp = sfc_readl(SFC_SR);
		if (reg_tmp & RECE_REQ) {
			sfc_writel(CLR_RREQ,SFC_SCR);
			if ((len - tmp_len) > THRESHOLD) {
				fifo_num = THRESHOLD;
			} else {
				fifo_num = len - tmp_len;
			}

			for (i = 0; i < fifo_num; i++) {
				*data = sfc_readl(SFC_DR);
				data++;
				tmp_len++;
			}
		}
		if (tmp_len == len)
			break;
	}

	reg_tmp = sfc_readl(SFC_SR);
	while (!(reg_tmp & END)){
		reg_tmp = sfc_readl(SFC_SR);
	}

	if ((sfc_readl(SFC_SR)) & END)
		sfc_writel(CLR_END,SFC_SCR);


	return 0;
}


static int sfc_write_data(unsigned int *data, unsigned int length)
{
	unsigned int tmp_len = 0;
	unsigned int fifo_num = 0;
	unsigned int i;
	unsigned int reg_tmp = 0;
	unsigned int  len = (length + 3) / 4 ;
	unsigned int time_out = 10000;

	while(1){
		reg_tmp = sfc_readl(SFC_SR);
		if (reg_tmp & TRAN_REQ) {
			sfc_writel(CLR_TREQ,SFC_SCR);
			if ((len - tmp_len) > THRESHOLD)
				fifo_num = THRESHOLD;
			else {
				fifo_num = len - tmp_len;
			}

			for (i = 0; i < fifo_num; i++) {
				sfc_writel(*data,SFC_DR);
				data++;
				tmp_len++;
			}
		}
		if (tmp_len == len)
			break;
	}

	reg_tmp = sfc_readl(SFC_SR);
	while (!(reg_tmp & END)){
		reg_tmp = sfc_readl(SFC_SR);
	}

	if ((sfc_readl(SFC_SR)) & END)
		sfc_writel(CLR_END,SFC_SCR);


	return 0;
}

int sfc_underrun()
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_SR);
	if(tmp & CLR_UNDER)
		return 1;
	else
		return 0;
}

int sfc_overrun()
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_SR);
	if(tmp & CLR_OVER)
		return 1;
	else
		return 0;
}

int rxfifo_rreq()
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_SR);
	if(tmp & CLR_RREQ)
		return 1;
	else
		return 0;
}
int txfifo_treq()
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_SR);
	if(tmp & CLR_TREQ)
		return 1;
	else
		return 0;
}
int sfc_end()
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_SR);
	if(tmp & CLR_END)
		return 1;
	else
		return 0;
}
unsigned int sfc_get_sta_rt()
{
	return sfc_readl(SFC_DEV_STA_RT);
}
unsigned int sfc_get_fsm()
{
	return sfc_readl(SFC_FSM);
}
void sfc_set_addr_length(int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~(ADDR_WIDTH_MSK);
	tmp |= (value << ADDR_WIDTH_OFFSET);
	sfc_writel(tmp, SFC_TRAN_CONF(channel));
}

void sfc_cmd_enble(int channel, unsigned int value)
{
	if(value == ENABLE) {
		unsigned int tmp;
		tmp = sfc_readl(SFC_TRAN_CONF(channel));
		tmp |= TRAN_CONF_CMDEN;
		sfc_writel(tmp, SFC_TRAN_CONF(channel));
	} else {
		unsigned int tmp;
		tmp = sfc_readl(SFC_TRAN_CONF(channel));
		tmp &= ~TRAN_CONF_CMDEN;
		sfc_writel(tmp, SFC_TRAN_CONF(channel));
	}
}

void sfc_data_en(int channel, unsigned int value)
{
	if(value == 1) {
		unsigned int tmp;
		tmp = sfc_readl(SFC_TRAN_CONF(channel));
		tmp |= TRAN_CONF_DATEEN;
		sfc_writel(tmp, SFC_TRAN_CONF(channel));
	} else {
		unsigned int tmp;
		tmp = sfc_readl(SFC_TRAN_CONF(channel));
		tmp &= ~TRAN_CONF_DATEEN;
		sfc_writel(tmp, SFC_TRAN_CONF(channel));
	}
}

void sfc_phase_format(int channel, unsigned int value)
{
	if(value == 1) {
		unsigned int tmp;
		tmp = sfc_readl(SFC_TRAN_CONF(channel));
		tmp |= TRAN_CONF_FMAT;
		sfc_writel(tmp, SFC_TRAN_CONF(channel));
	} else {
		unsigned int tmp;
		tmp = sfc_readl( SFC_TRAN_CONF(channel));
		tmp &= ~TRAN_CONF_FMAT;
		sfc_writel(tmp, SFC_TRAN_CONF(channel));
	}
}

void sfc_write_cmd(int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~TRAN_CONF_CMD_MSK;
	tmp |= value;
	sfc_writel(tmp, SFC_TRAN_CONF(channel));
}

void sfc_dev_addr(int channel, unsigned int value)
{
	sfc_writel(value, SFC_DEV_ADDR(channel));
}


void sfc_dev_data_dummy_bytes(int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~TRAN_CONF_DMYBITS_MSK;
	tmp |= value << DMYBITS_OFFSET;
	sfc_writel(tmp, SFC_TRAN_CONF(channel));
}

void sfc_dev_addr_plus(int channel, unsigned int value)
{
	sfc_writel(value, SFC_DEV_ADDR_PLUS(channel));
}

void sfc_dev_pollen(int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = sfc_readl(SFC_TRAN_CONF(channel));
	if(value == 1)
		tmp |= TRAN_CONF_POLLEN;
	else
		tmp &= ~(TRAN_CONF_POLLEN);

	sfc_writel(tmp, SFC_TRAN_CONF(channel));
}

void sfc_dev_sta_exp(unsigned int value)
{
	sfc_writel(value, SFC_DEV_STA_EXP);
}

void sfc_dev_sta_msk(unsigned int value)
{
	sfc_writel(value, SFC_DEV_STA_MSK);
}

void sfc_set_mem_addr(unsigned int addr )
{
	sfc_writel(addr, SFC_MEM_ADDR);
}
static int sfc_start_transfer()
{
	sfc_start();
	return 0;
}
static void sfc_phase_transfer(struct sfc_transfer *
		transfer,int channel)
{
	sfc_flush_fifo();
	sfc_set_addr_length(channel,transfer->addr_len);
	sfc_cmd_enble(channel,ENABLE);
	sfc_write_cmd(channel,transfer->cmd_info->cmd);
	sfc_dev_data_dummy_bytes(channel,transfer->data_dummy_bits);
	sfc_data_en(channel,transfer->cmd_info->dataen);
	sfc_dev_addr(channel,transfer->addr);
	sfc_dev_addr_plus(channel,transfer->addr_plus);
	sfc_mode(channel,transfer->sfc_mode);
	sfc_phase_format(channel,0);/*default 0,dummy bits is blow the addr*/

}
static void common_cmd_request_transfer(struct sfc_transfer *transfer,int channel)
{
	sfc_phase_transfer(transfer,channel);
	sfc_dev_sta_exp(0);
	sfc_dev_sta_msk(0);
	sfc_dev_pollen(channel,DISABLE);
}

static void poll_cmd_request_transfer(struct sfc_transfer *transfer,int channel)
{
	struct cmd_info *cmd = transfer->cmd_info;
	sfc_phase_transfer(transfer,channel);
	sfc_dev_sta_exp(cmd->sta_exp);
	sfc_dev_sta_msk(cmd->sta_msk);
	sfc_dev_pollen(channel,ENABLE);
}
static void sfc_glb_info_config(struct sfc_transfer *transfer, void *data)
{
	sfc_transfer_direction(transfer->direction);
	if((transfer->ops_mode == DMA_OPS)){
		sfc_set_length(transfer->len);
		if(transfer->direction == GLB_TRAN_DIR_READ) {
			flush_cache_all();
		} else {
			flush_cache_all();
		}
		/*sfc_set_mem_addr( GET_PHYADDR(transfer->data));*/
		sfc_set_mem_addr((unsigned int)(transfer->data)&0x1fffffff);
		sfc_transfer_mode(DMA_MODE);
	}else{
		sfc_set_length(transfer->len);
		sfc_set_mem_addr( 0);
		sfc_transfer_mode(SLAVE_MODE);
	}
}
#ifdef DEBUG
void  dump_transfer(struct sfc_transfer *xfer,int num)
{
	printf("\n");
	printf("cmd[%d].cmd = 0x%02x\n",num,xfer->cmd_info->cmd);
	printf("cmd[%d].addr_len = %d\n",num,xfer->addr_len);
	printf("cmd[%d].dummy_byte = %d\n",num,xfer->data_dummy_bits);
	printf("cmd[%d].dataen = %d\n",num,xfer->cmd_info->dataen);
	printf("cmd[%d].sta_exp = %d\n",num,xfer->cmd_info->sta_exp);
	printf("cmd[%d].sta_msk = %d\n",num,xfer->cmd_info->sta_msk);


	printf("transfer[%d].addr = 0x%08x\n",num,xfer->addr);
	printf("transfer[%d].len = %x\n",num,xfer->len);
	printf("transfer[%d].data = 0x%p\n",num,xfer->data);
	printf("transfer[%d].direction = %d\n",num,xfer->direction);
	printf("transfer[%d].sfc_mode = %d\n",num,xfer->sfc_mode);
	printf("transfer[%d].ops_mode = %d\n",num,xfer->ops_mode);
}
#endif
int sfc_sync(void *data ,struct sfc_message *message)
{
	struct sfc_transfer *xfer;
	int phase_num = 0,ret = 0;
	int llen = 0;
	int reg_tmp = 0;


	list_for_each_entry(xfer, &message->transfers, transfer_list) {
		if(xfer->cmd_info->sta_msk == 0){
			common_cmd_request_transfer(xfer,phase_num);
		}else{
			poll_cmd_request_transfer(xfer,phase_num);
		}
		if(xfer->addr_len || xfer->len)
			sfc_glb_info_config(xfer, data);
		phase_num++;
		message->actual_length += xfer->len;
		llen = xfer->len;
	}
	sfc_set_phase_num(phase_num);
	ret = sfc_start_transfer();
	xfer = list_entry((&message->transfers)->next, typeof(*xfer), transfer_list);
	if(xfer->ops_mode == DMA_OPS) {
		if ((sfc_readl(SFC_SR)) & END)
			sfc_writel(CLR_END,SFC_SCR);
	} else {
		if(xfer->cmd_info->dataen == ENABLE) {
			if(xfer->direction == GLB_TRAN_DIR_READ) {
				sfc_read_data(data,(llen ));
			} else {
				sfc_write_data(data,(llen));
			}
		} else {
			reg_tmp = sfc_readl(SFC_SR);
			while (!(reg_tmp & END)){
				reg_tmp = sfc_readl(SFC_SR);
			}

			if ((sfc_readl(SFC_SR)) & END)
				sfc_writel(CLR_END,SFC_SCR);
		}
	}
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

int jz_sfc_init_setup()
{
	sfc_clk_set();
	sfc_init();
	sfc_stop();

	/*set hold high*/
	sfc_hold_invalid_value(1);
	/*set wp high*/
	sfc_wp_invalid_value(1);

	sfc_threshold(THRESHOLD);
	/*config the sfc pin init state*/
	sfc_clock_phase(0);
	sfc_clock_polarity( 0);
	sfc_ce_invalid_value(1);

	set_flash_timing(tCHSH, tSLCH, tSHSL_RD, tSHSL_WR);

	sfc_transfer_mode(SLAVE_MODE);
	if(sfc_clk_rate >= 100000000){
		sfc_smp_delay(DEV_CONF_HALF_CYCLE_DELAY);
	}


	return 0;
}
