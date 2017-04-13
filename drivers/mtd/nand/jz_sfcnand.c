/*
 * jz mtd spi nand driver probe interface
 *
 *  Copyright (C) 2013 Ingenic Semiconductor Co., LTD.
 *  Author: cli <chen.li@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later versio.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#include <config.h>
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <nand.h>
#include <spi.h>
#include <spi_flash.h>
#include <linux/list.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <asm/arch/spi.h>
#include <asm/arch/sfc.h>
#include <asm/io.h>
#include <ingenic_nand_mgr/nand_param.h>
#include "../spi/spi_flash_internal.h"
#include "../../spi/jz_spi.h"
#include "jz_spinand.h"
#include <asm/arch/sfc-jz.h>

#include <cloner/cloner.h>
#include <asm/arch/cpm.h>
#include <asm/arch/clk.h>
#define IDCODE_CONT_LEN 0
#define IDCODE_PART_LEN 5
#ifdef MTDIDS_DEFAULT
static const char *const mtdids_default = MTDIDS_DEFAULT;
#else
static const char *const mtdids_default = "nand0:nand";
#endif


unsigned int sfc_rate = 0;
unsigned short column_cmdaddr_bits=24;/* read from cache ,the bits of cmd + addr */

struct nand_param_from_burner nand_param_from_burner;
#define SIZE_UBOOT  0x100000    /* 1M */
#define SIZE_KERNEL 0x800000    /* 8M */
#define SIZE_ROOTFS (0x100000 * 40)        /* -1: all of left */

static struct jz_spinand_partition jz_mtd_spinand_partition[] = {
        {
                .name =     "uboot",
                .offset =   0,
                .size =     SIZE_UBOOT,
                .manager_mode = MTD_MODE,
        },
	{
                .name =     "kernel",
                .offset =   SIZE_UBOOT,
                .size =     SIZE_KERNEL,
                .manager_mode = MTD_MODE,
        },
        {
                .name   =       "rootfs",
                .offset =   SIZE_UBOOT + SIZE_KERNEL,
                .size   =   SIZE_ROOTFS,
                .manager_mode = UBI_MANAGER,
                //.manager_mode = MTD_MODE,
        },
};
/* wait time before read status (us) for spi nand */
//static int t_reset = 500;
static int t_read  = 120;
static int t_write = 700;
static int t_erase = 5000;


static int mode = 0;
struct jz_sfc {
	unsigned int  addr;
	unsigned int  len;
	unsigned int  cmd;
	unsigned int  addr_plus;
	unsigned int  sfc_mode;
	unsigned char daten;
	unsigned char addr_len;
	unsigned char pollen;
	unsigned char phase;
	unsigned char dummy_byte;
};


static uint32_t jz_sfc_readl(unsigned int offset)
{
	return readl(SFC_BASE + offset);
}

static void jz_sfc_writel(unsigned int value, unsigned int offset)
{
	writel(value, SFC_BASE + offset);
}

static void dump_sfc_reg()
{
	int i = 0;
	printf("SFC_GLB			:%x\n", jz_sfc_readl(SFC_GLB ));
	printf("SFC_DEV_CONF	:%x\n", jz_sfc_readl(SFC_DEV_CONF ));
	printf("SFC_DEV_STA_RT	:%x\n", jz_sfc_readl(SFC_DEV_STA_RT ));
	printf("SFC_DEV_STA_MSK	:%x\n", jz_sfc_readl(SFC_DEV_STA_MSK ));
	printf("SFC_TRAN_LEN		:%x\n", jz_sfc_readl(SFC_TRAN_LEN ));

	for(i = 0; i < 6; i++)
		printf("SFC_TRAN_CONF(%d)	:%x\n", i,jz_sfc_readl(SFC_TRAN_CONF(i)));

	for(i = 0; i < 6; i++)
		printf("SFC_DEV_ADDR(%d)	:%x\n", i,jz_sfc_readl(SFC_DEV_ADDR(i)));

	printf("SFC_MEM_ADDR :%x\n", jz_sfc_readl(SFC_MEM_ADDR));
	printf("SFC_TRIG	 :%x\n", jz_sfc_readl(SFC_TRIG));
	printf("SFC_SR		 :%x\n", jz_sfc_readl(SFC_SR));
	printf("SFC_SCR		 :%x\n", jz_sfc_readl(SFC_SCR));
	printf("SFC_INTC	 :%x\n", jz_sfc_readl(SFC_INTC));
	printf("SFC_FSM		 :%x\n", jz_sfc_readl(SFC_FSM ));
	printf("SFC_CGE		 :%x\n", jz_sfc_readl(SFC_CGE ));

}

static void sfc_transfer_direction(int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = jz_sfc_readl(SFC_GLB);
		tmp &= ~TRAN_DIR;
		jz_sfc_writel(tmp,SFC_GLB);
	} else {
		unsigned int tmp;
		tmp = jz_sfc_readl(SFC_GLB);
		tmp |= TRAN_DIR;
		jz_sfc_writel(tmp,SFC_GLB);
	}
}

static void sfc_set_length(int value)
{
	jz_sfc_writel(value,SFC_TRAN_LEN);
}
static void sfc_set_addr_length(int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~(ADDR_WIDTH_MSK);
	tmp |= (value << ADDR_WIDTH_OFFSET);
	jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
}

static void sfc_cmd_en(int channel, unsigned int value)
{
	if(value == 1) {
		unsigned int tmp;
		tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
		tmp |= CMDEN;
		jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
	} else {
		unsigned int tmp;
		tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
		tmp &= ~CMDEN;
		jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
	}
}

static void sfc_data_en(int channel, unsigned int value)
{
	if(value == 1) {
		unsigned int tmp;
		tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
		tmp |= DATEEN;
		jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
	} else {
		unsigned int tmp;
		tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
		tmp &= ~DATEEN;
		jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
	}
}

static void sfc_write_cmd(int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~CMD_MSK;
	tmp |= value;
	jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
}

static void sfc_dev_addr(int channel, unsigned int value)
{
	jz_sfc_writel(value, SFC_DEV_ADDR(channel));
}

static void sfc_dev_addr_plus(int channel, unsigned int value)
{
	jz_sfc_writel(value,SFC_DEV_ADDR_PLUS(channel));
}
static void sfc_dev_addr_dummy_bytes(int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~TRAN_CONF_DMYBITS_MSK;
	tmp |= (value << TRAN_CONF_DMYBITS_OFFSET);
	jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
}
static void sfc_set_mode(int channel, int value)
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~(TRAN_MODE_MSK);
	tmp |= (value << TRAN_MODE_OFFSET);
	jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
}



static void sfc_set_transfer(struct jz_sfc *hw,int dir)
{
	if(dir == 1)
		sfc_transfer_direction(GLB_TRAN_DIR_WRITE);
	else
		sfc_transfer_direction(GLB_TRAN_DIR_READ);
	sfc_set_length(hw->len);
	sfc_set_addr_length(0, hw->addr_len);
	sfc_cmd_en(0, 0x1);
	sfc_data_en(0, hw->daten);
	sfc_write_cmd(0, hw->cmd);
	sfc_dev_addr(0, hw->addr);
	sfc_dev_addr_plus(0, hw->addr_plus);
	sfc_dev_addr_dummy_bytes(0,hw->dummy_byte);
	sfc_set_mode(0,hw->sfc_mode);

}
static int sfc_read_data(unsigned int *data, unsigned int length)
{
	unsigned int tmp_len = 0;
	unsigned int fifo_num = 0;
	unsigned int i;
	unsigned int reg_tmp = 0;
	unsigned int  len = (length + 3) / 4 ;
	unsigned int time_out = 1000;

	while(1){
		reg_tmp = jz_sfc_readl(SFC_SR);
		if (reg_tmp & RECE_REQ) {
			jz_sfc_writel(CLR_RREQ,SFC_SCR);
			if ((len - tmp_len) > THRESHOLD)
				fifo_num = THRESHOLD;
			else {
				fifo_num = len - tmp_len;
			}

			for (i = 0; i < fifo_num; i++) {
				*data = jz_sfc_readl(SFC_DR);
				data++;
				tmp_len++;
			}
		}
		if (tmp_len == len)
			break;
	}

	reg_tmp = jz_sfc_readl(SFC_SR);
	while (!(reg_tmp & END)){
		reg_tmp = jz_sfc_readl(SFC_SR);
	}

	if ((jz_sfc_readl(SFC_SR)) & END)
		jz_sfc_writel(CLR_END,SFC_SCR);


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
		reg_tmp = jz_sfc_readl(SFC_SR);
		if (reg_tmp & TRAN_REQ) {
			jz_sfc_writel(CLR_TREQ,SFC_SCR);
			if ((len - tmp_len) > THRESHOLD)
				fifo_num = THRESHOLD;
			else {
				fifo_num = len - tmp_len;
			}

			for (i = 0; i < fifo_num; i++) {
				jz_sfc_writel(*data,SFC_DR);
				data++;
				tmp_len++;
			}
		}
		if (tmp_len == len)
			break;
	}

	reg_tmp = jz_sfc_readl(SFC_SR);
	while (!(reg_tmp & END)){
		reg_tmp = jz_sfc_readl(SFC_SR);
	}

	if ((jz_sfc_readl(SFC_SR)) & END)
		jz_sfc_writel(CLR_END,SFC_SCR);


	return 0;
}



void sfc_send_cmd(unsigned char *cmd,unsigned int len,unsigned int addr ,unsigned addr_len,unsigned dummy_byte,unsigned int daten,unsigned char dir)
{
	struct jz_sfc sfc;
	unsigned int reg_tmp = 0;
	sfc.cmd = *cmd;
	sfc.addr_len = addr_len;
	//sfc.addr = ((*addr << 16) &0x00ff0000) | ((*(addr + 1) << 8)&0x0000ff00) |((*(addr + 2))&0xff);
	sfc.addr = addr;
	sfc.addr_plus = 0;
	sfc.dummy_byte = dummy_byte;
	sfc.daten = daten;
	sfc.len = len;

	if((daten == 1)&&(addr_len != 0)){
		sfc.sfc_mode = mode;
	}else{
		sfc.sfc_mode = 0;
	}
	sfc_set_transfer(&sfc,dir);
	jz_sfc_writel(1 << 2,SFC_TRIG);
	jz_sfc_writel(START,SFC_TRIG);

	/*this must judge the end status*/
	if((daten == 0)){
		reg_tmp = jz_sfc_readl(SFC_SR);
		while (!(reg_tmp & END)){
			reg_tmp = jz_sfc_readl(SFC_SR);
		}

		if ((jz_sfc_readl(SFC_SR)) & END)
			jz_sfc_writel(CLR_END,SFC_SCR);
	}

}

int sfc_nand_write_data(unsigned int *data,unsigned int length)
{
	return sfc_write_data(data,length);
}
int sfc_nand_read_data(unsigned int *data, unsigned int length)
{
	 return sfc_read_data(data,length);
}


void sfc_for_nand_init(int sfc_quad_mode)
{
	unsigned int i;
	volatile unsigned int tmp;
	sfc_rate = 100000000;
	clk_set_rate(SSI, sfc_rate);

	tmp = jz_sfc_readl(SFC_GLB);
	tmp &= ~(TRAN_DIR | OP_MODE );
	tmp |= WP_EN;
	jz_sfc_writel(tmp,SFC_GLB);
	tmp = jz_sfc_readl(SFC_DEV_CONF);
	tmp &= ~(CMD_TYPE | CPHA | CPOL | SMP_DELAY_MSK |
				THOLD_MSK | TSETUP_MSK | TSH_MSK);
	tmp |= (CEDL | HOLDDL | WPDL | 1 << SMP_DELAY_OFFSET);
	jz_sfc_writel(tmp,SFC_DEV_CONF);
	for (i = 0; i < 6; i++) {
		jz_sfc_writel((jz_sfc_readl(SFC_TRAN_CONF(i))& (~(TRAN_MODE_MSK | FMAT))),SFC_TRAN_CONF(i));
	     if(sfc_quad_mode==1)
	     {
		unsigned int temp=0;
		temp=jz_sfc_readl(SFC_TRAN_CONF(i));
		temp&=~(7<<29);
		temp|=(5<<29);
		jz_sfc_writel(temp,SFC_TRAN_CONF(i));
	     }
	}
	jz_sfc_writel((CLR_END | CLR_TREQ | CLR_RREQ | CLR_OVER | CLR_UNDER),SFC_INTC);
	jz_sfc_writel(0,SFC_CGE);
	tmp = jz_sfc_readl(SFC_GLB);
	tmp &= ~(THRESHOLD_MSK);
	tmp |= (THRESHOLD << THRESHOLD_OFFSET);
	jz_sfc_writel(tmp,SFC_GLB);

}

static struct nand_ecclayout gd5f_ecc_layout_128 = {
	.oobavail = 0,
};


int sfc_nand_erase(struct mtd_info *mtd,int addr)
{
	unsigned char cmd[COMMAND_MAX_LENGTH];
	int erase_cmd;
	int page = addr / mtd->writesize;
	int block_size = mtd->erasesize;

	switch(block_size){
		case 4 * 1024:
			erase_cmd = CMD_ERASE_4K;
			break;
		case 32 * 1024:
			erase_cmd = CMD_ERASE_32K;
			break;
		case 64 * 1024:
			erase_cmd = CMD_ERASE_64K;
			break;
		case 128 * 1024:
			erase_cmd = CMD_ERASE_128K;
			break;
		case 256 * 1024:
			erase_cmd = CMD_ERASE_128K;
			break;
		default:
			printf("WARNING: don't support the erase size !\n");
			break;
	}

	/* the paraterms is
	* cmd , len, addr,addr_len
	* dummy_byte, daten
	* dir 0,read 1.write
	*
	* */
	volatile unsigned int x;
	cmd[0] = CMD_WREN;//write en
	sfc_send_cmd(&cmd[0],0,0,0,0,0,0);
	memset(cmd,COMMAND_MAX_LENGTH,0);
	cmd[0]=erase_cmd;//erase
	sfc_send_cmd(&cmd[0],0,page,3,0,0,0);
	memset(cmd,COMMAND_MAX_LENGTH,0);

	cmd[0]=CMD_GET_FEATURE;//get feature
	sfc_send_cmd(&cmd[0],1,0xc0,1,0,1,0);
	sfc_nand_read_data(&x,1);
	x=x&0x000000ff;
	while(x & 0x1)
	{
		x=0;
		sfc_send_cmd(&cmd[0],1,0xc0,1,0,1,0);
		sfc_nand_read_data(&x,1);
	}
	if(x & E_FAIL)
		return -1;

	return 0;
}
static int sfc_nand_write(struct mtd_info *mtd,loff_t addr,int column,size_t len,u_char *buf,size_t *retlen)
{
	unsigned char state, cmd[COMMAND_MAX_LENGTH];
	int page_size = mtd->writesize;
	int page;// = addr / page_size;
	int wlen,i,write_num;
	int ops_len;
	u_char *buffer = buf;
	page = addr/page_size;

	write_num = (len + page_size - 1) / page_size;
	for(i = 0; i < write_num; i++)
	{
		if(len >= page_size)
			wlen = page_size;
		else
			wlen = len;

		/* the paraterms is
		* cmd , datelen,
		*addr,addr_len
		* dummy_byte, daten
		* dir 0,read 1.write
		*
		* */

		cmd[0] = CMD_WREN;
		sfc_send_cmd(&cmd[0],0,0,0,0,0,0);

		cmd[0]=CMD_PRO_LOAD;//02h
		sfc_send_cmd(&cmd[0],wlen,column,2,0,1,1);
		ops_len = wlen;
		sfc_nand_write_data(buffer,ops_len);
		buffer += ops_len;

		cmd[0] = CMD_PE;
		sfc_send_cmd(&cmd[0],0,page,3,0,0,0);
		udelay(t_write);

		state=0;
		cmd[0] = CMD_GET_FEATURE;
		sfc_send_cmd(&cmd[0],1,FEATURE_ADDR,1,0,1,0);
		sfc_nand_read_data(&state,1);
		while(state & 0x1) ///////////////////////////////////////////
		{
			cmd[0] = CMD_GET_FEATURE;
			sfc_send_cmd(&cmd[0],1,FEATURE_ADDR,1,0,1,0);
			sfc_nand_read_data(&state,1);
		}

		if(state & P_FAIL){
			printf("WARNING: write fail !\n");
			*retlen = i * wlen;
			return -EIO;
		}
		*retlen = len;
		len -= wlen;
		page++;
	}
	return 0;
}
static int sfc_nand_read(struct mtd_info *mtd,loff_t addr,int column,size_t len,u_char *buf)
{
        int page_size = mtd->writesize;
        int page = addr / page_size;
        int i,read_num,rlen;
	u_char *buffer=buf;
        size_t page_overlength;
        size_t ops_addr;
        size_t ops_len;
        int ret;

        if(column){
                ops_addr = (unsigned int)addr;
                ops_len = len;

                if(len <= (page_size - column))
                        page_overlength = len;
                else
                        page_overlength = page_size - column;/*random read but len over a page */
                while(ops_addr < addr + len){
                        page = ops_addr / page_size;
                        if(page_overlength){
                                ret = sfc_nand_read_page(buffer,page,column,page_overlength);
                                if(ret < 0)
                                        return ret;
                                ops_len -= page_overlength;
                                buffer += page_overlength;
                                ops_addr += page_overlength;
                                page_overlength = 0;
                        }else{
                                column = 0;
                                if(ops_len >= page_size)
                                        rlen = page_size;
                                else
                                        rlen = ops_len;

                                ret = sfc_nand_read_page(buffer,page,column,rlen);
                                if(ret < 0)
                                        return ret;

                                buffer += rlen;
                                ops_len -= rlen;
                                ops_addr += rlen;
                        }
                }
       }else{
                read_num = (len + page_size - 1) / page_size;
                page = addr / page_size;
                for(i = 0; i < read_num; i++){
                        if(len >= page_size)
                                rlen = page_size;
                        else
                                rlen = len;

                        ret = sfc_nand_read_page(buffer,page,column,rlen);
                        if(ret < 0)
                                return ret;

                        buffer += rlen;
                        len -= rlen;
                        page++;
                }
        }
        return 0;
}
int sfc_nand_read_page(u_char *buffer,int page,int column,size_t rlen)
{
	unsigned char cmd[COMMAND_MAX_LENGTH];
	volatile unsigned int read_buf;

	cmd[0]=CMD_PARD;//13h read to cache
	sfc_send_cmd(&cmd[0],0,page,3,0,0,0);

	cmd[0]=CMD_GET_FEATURE;//get feature 0x0f
	sfc_send_cmd(&cmd[0],1,FEATURE_ADDR,1,0,1,0);
	sfc_nand_read_data(&read_buf,1);
	while(read_buf & 0x1)
	{
		cmd[0]=CMD_GET_FEATURE;//get feature
		sfc_send_cmd(&cmd[0],1,FEATURE_ADDR,1,0,1,0);
		sfc_nand_read_data(&read_buf,1);
	}
	if((read_buf & 0x30) == 0x20) {
		printf("%s %d read error pageid = %d!!!\n",__func__,__LINE__,page);
		return -1;
	}
	switch(column_cmdaddr_bits){
		printf("column_cmdaddr_bits=%d\n", column_cmdaddr_bits);
		case 24:
			cmd[0]=CMD_R_CACHE;//03h read from cache
			column=(column<<8)&0xffffff00;
			sfc_send_cmd(&cmd[0],rlen,column,2,8,1,0);
			sfc_nand_read_data(buffer,rlen);
			break;
		case 32:
			cmd[0]=CMD_FR_CACHE;//0bh read from cache
			column=(column<<8)&0xffffff00;
			sfc_send_cmd(&cmd[0],rlen,column,2,8,1,0);
			sfc_nand_read_data(buffer,rlen);
			break;
		default:
			printf("can't support the column addr format !!!\n");
			break;
	}

	return 0;
}
static int sfcnand_write_oob(struct mtd_info *mtd,loff_t addr,struct mtd_oob_ops *ops)
{
	unsigned char cmd[COMMAND_MAX_LENGTH];
	int page = addr / mtd->writesize;
	int ret;
	size_t retlen;

	cmd[0]=CMD_PARD;//get feature
	sfc_send_cmd(&cmd[0],0,page,3,0,0,0);
	udelay(t_read);

	ret = sfc_nand_write(mtd,addr,mtd->writesize,ops->ooblen,ops->oobbuf,&retlen);
	if(ret){
		printf("WARNING: Mark bad block fail !\n");
		return ret;
	}
	return 0;
}
static int sfcnand_block_isbad(struct mtd_info *mtd,loff_t ofs) ;
static int sfcnand_block_markbad(struct mtd_info *mtd,loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	int ret;

	ret = sfcnand_block_isbad(mtd, ofs);
	if (ret) {
		/* If it was bad already, return success and do nothing */
		if (ret > 0)
			return 0;
		return ret;
	}

	return chip->block_markbad(mtd, ofs);
}
static int spinand_read_oob(struct mtd_info *mtd,loff_t addr,struct mtd_oob_ops *ops);
int sfcnand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int block_size = mtd->erasesize;
	int block,addr,ret;
	addr = instr->addr;

	ret = sfc_nand_erase(mtd,addr);
	if(ret){
		printf("WARNING: block %d erase fail !\n",addr / mtd->erasesize);

		ret = sfcnand_block_markbad(mtd,addr);
		if(ret){
			printf("mark bad block error, there will occur error,so exit !\n");
			return -1;
		}
	}
	instr->state = MTD_ERASE_DONE;
	return ret;
}
static int sfcnand_read_oob(struct mtd_info *mtd,loff_t addr,struct mtd_oob_ops *ops)
{
	int len = ops->ooblen;
	int column = mtd->writesize;
	unsigned char cmd[COMMAND_MAX_LENGTH];
	volatile unsigned int read_buf;
	int page_size = mtd->writesize;
	int page = addr / page_size;
	u_char *buffer = ops->oobbuf;
	/* the paraterms is
	* cmd , len, addr,addr_len
	* dummy_byte, daten
	* dir 0,read 1.write
	*
	* */
	cmd[0] = CMD_PARD;//write en
	sfc_send_cmd(&cmd[0],0,page,3,0,0,0);
	udelay(t_read);

	cmd[0]=CMD_GET_FEATURE;//get feature
	sfc_send_cmd(&cmd[0],1,FEATURE_ADDR,1,0,1,0);
	sfc_nand_read_data(&read_buf,1);



	while(read_buf & 0x1)
	{
		cmd[0]=CMD_GET_FEATURE;//get feature
		sfc_send_cmd(&cmd[0],1,FEATURE_ADDR,1,0,1,0);
		sfc_nand_read_data(&read_buf,1);
	}
	if((read_buf & 0x30) == 0x20) {
		printf("%s %d read error pageid = %d!!!\n",__func__,__LINE__,page);
		return 1;
	}
        switch(column_cmdaddr_bits){
        	case 24:
			cmd[0]=CMD_R_CACHE;//get feature
			column=(column<<8)&0xffffff00;
			sfc_send_cmd(&cmd[0],len,column,3,0,1,0);
			sfc_nand_read_data(buffer,len);
			break;
		case 32:
			cmd[0]=CMD_FR_CACHE;//get feature
			column=(column<<8)&0xffffff00;
			sfc_send_cmd(&cmd[0],len,column,4,0,1,0);
			sfc_nand_read_data(buffer,len);
			break;
		default:
			printf("can't support the column addr format !!!\n");
			break;
	}

	return 0;
}
static int badblk_check(int len,unsigned char *buf)
{
	int i,bit0_cnt = 0;
	unsigned short *check_buf = (unsigned short *)buf;

	if(check_buf[0] != 0xff){
		for(i = 0; i < len * 8; i++){
			if(!((check_buf[0] >> i) & 0x1))
				bit0_cnt++;
		}
	}
	if(bit0_cnt > 6 * len)
		return 1; // is bad blk

	return 0;
}
static int sfcnand_block_checkbad(struct mtd_info *mtd, loff_t ofs,int getchip,int allowbbt)
{
	struct nand_chip *chip = mtd->priv;

	if (!(chip->options & NAND_BBT_SCANNED)) {
		chip->options |= NAND_BBT_SCANNED;
		chip->scan_bbt(mtd);
	}
	if (!chip->bbt)
		return chip->block_bad(mtd, ofs,getchip);

	/* Return info from the table */
	return nand_isbad_bbt(mtd, ofs, allowbbt);
}

static int sfcnand_block_isbad(struct mtd_info *mtd,loff_t ofs)
{
	int ret;
	ret = sfcnand_block_checkbad(mtd, ofs,1, 0);
	return ret;
}
static int jz_sfcnand_block_bad_check(struct mtd_info *mtd, loff_t ofs,int getchip)
{
	int column = mtd->writesize;
	int check_len = 2;
	unsigned char check_buf[] = {0xaa,0xaa};
	unsigned char  cmd[COMMAND_MAX_LENGTH];
	struct mtd_oob_ops ops;

	ops.oobbuf = check_buf;
	ops.ooblen = check_len;
	sfcnand_read_oob(mtd,ofs,&ops);

	if(badblk_check(check_len,check_buf))
		return 1;
	return 0;
}
static int sfcnand_read(struct mtd_info *mtd,loff_t addr,size_t len,size_t *retlen,u_char *buf)
{
	int ret;

	ret = sfc_nand_read(mtd,addr,0,len,buf);
	if(ret)
		*retlen = ret;
	else
		*retlen = len;

	return ret;
}
static int sfcnand_write(struct mtd_info *mtd,loff_t addr,size_t len,size_t *retlen, u_char *buf)
{
	int ret,i,n=0;
	ret = sfc_nand_write(mtd,addr,0,len,buf,retlen);

	return ret;
}
static void mtd_sfcnand_init(struct mtd_info *mtd)
{
	mtd->_erase = sfcnand_erase;
	mtd->_point = NULL;
	mtd->_unpoint = NULL;
	mtd->_read = sfcnand_read;
	mtd->_write = sfcnand_write;
	mtd->_read_oob = sfcnand_read_oob;
	mtd->_write_oob = sfcnand_write_oob;
	mtd->_lock = NULL;
	mtd->_unlock = NULL;
	mtd->_block_isbad = sfcnand_block_isbad;
	mtd->_block_markbad = sfcnand_block_markbad;
}


static void jz_sfcnand_ext_init(void )
{
	unsigned char cmd[COMMAND_MAX_LENGTH];
	unsigned int x=0;
	/* disable write protect */
	unsigned int add;
	cmd[0]=0x1f;//set feature
	add=0xa0;

	sfc_send_cmd(&cmd[0],1,add,1,0,1,1);
	sfc_nand_write_data(&x,1);

	cmd[0]=0x1f;//set feature
	add=0xb0;
	x=0x10;
	sfc_send_cmd(&cmd[0],1,add,1,0,1,1);
	sfc_nand_write_data(&x,1);

	x=0;
	cmd[0]=CMD_GET_FEATURE;//get feature
	add=0xa0;
	sfc_send_cmd(&cmd[0],1,add,1,0,1,0);
	sfc_nand_read_data(&x,1);
	x=x&0x000000ff;
	printf("read status 0xa0 : %x\n", x);

	x=0;
	cmd[0]=CMD_GET_FEATURE;//get feature
	add=0xb0;
	sfc_send_cmd(&cmd[0],1,add,1,0,1,0);
	sfc_nand_read_data(&x,1);
	x=x&0x000000ff;
	printf("read status 0xb0 : %x\n", x);

}
static int jz_sfcnand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	uint8_t buf[2] = { 0, 0 };
	int block, res, ret = 0, i = 0;
	int write_oob = !(chip->bbt_options & NAND_BBT_NO_OOB_BBM);

	/* Write bad block marker to OOB */
	if (write_oob) {
		struct mtd_oob_ops ops;
		loff_t wr_ofs = ofs;

		ops.datbuf = NULL;
		ops.oobbuf = buf;
		ops.ooboffs = chip->badblockpos;
		if (chip->options & NAND_BUSWIDTH_16) {
			ops.ooboffs &= ~0x01;
			ops.len = ops.ooblen = 2;
		} else {
			ops.len = ops.ooblen = 1;
		}
		ops.mode = MTD_OPS_PLACE_OOB;

		/* Write to first/last page(s) if necessary */
		if (chip->bbt_options & NAND_BBT_SCANLASTPAGE)
			wr_ofs += mtd->erasesize - mtd->writesize;
		do {
			res = sfcnand_write_oob(mtd, wr_ofs, &ops);
			if (!ret)
				ret = res;

			i++;
			wr_ofs += mtd->writesize;
		} while ((chip->bbt_options & NAND_BBT_SCAN2NDPAGE) && i < 2);
	}

	/* Update flash-based bad block table */
	if (chip->bbt_options & NAND_BBT_USE_FLASH) {
		res = nand_update_bbt(mtd, ofs);
		if (!ret)
			ret = res;
	}

	if (!ret)
		mtd->ecc_stats.badblocks++;

	return ret;

}

static struct jz_spi_support_from_burner *sfc_nandflash_probe(u8 *idcode,struct nand_param_from_burner *param_array)
{
	int i;
	struct jz_spi_support_from_burner *params=param_array->addr;
	for (i = 0; i < param_array->para_num; i++) {
		if ( params->id_manufactory == ((idcode[0]<<8)|idcode[1]))
			break;
		params++;
	}

	if (i == param_array->para_num) {
		return NULL;
	}
#if 0
	printf("*******************************************************************\n");
	printf("id=0x%08x\n",params->id_manufactory);
	printf("name=%s\n",params->name);
	printf("page_size=%d\n",params->page_size);
	printf("oobsize=%d\n",params->oobsize);
	printf("sector_size=%d\n",params->sector_size);
	printf("block_size=%d\n",params->block_size);
	printf("size=%d\n",params->size);
	printf("page_num=%d\n",params->page_num);
	printf("tRD_maxbusy=%d\n",params->tRD_maxbusy);
	printf("tPROG_maxbusy=%d\n",params->tPROG_maxbusy);
	printf("column_cmdaddr_bits=%d\n",params->column_cmdaddr_bits);
	printf("*******************************************************************\n");
#endif
	return params;
}

static void jz_sfc_get_param(char *buffer,struct nand_param_from_burner *param)
{
        char *member_addr;
	member_addr=buffer+sizeof(int32_t);  //magic
	param->version=*(int *)member_addr;
	member_addr+=sizeof(param->version);
	param->flash_type=*(int *)member_addr;
	member_addr+=sizeof(param->flash_type);
	param->para_num=*(int *)member_addr;
	member_addr+=sizeof(param->para_num);
        param->addr=member_addr;
	member_addr+=param->para_num*sizeof(struct jz_spi_support_from_burner);
        param->partition_num=*(int *)member_addr;
	member_addr+=sizeof(param->partition_num);
        param->partition=member_addr;
}
static int get_page_size(int page,int column)
{
        char buffer[100];
	int pagesize=0;
        sfc_nand_read_page(buffer,page,column,100);
        pagesize=buffer[SPL_TYPE_FLAG_LEN+5]*1024;
	return pagesize;
}
static int32_t get_nand_magic(int page,int column)
{
        char buffer[column+sizeof(int)];
        int32_t nand_magic=0;
        sfc_nand_read_page(buffer,page,0,column+sizeof(int));
        nand_magic=*(int32_t *)(buffer+column);
        return nand_magic;
}
static int read_spinand_param(char *buffer,int ptcout,struct nand_param_from_burner *param,int pagesize)
{
        int offset= ptcout % pagesize;
        sfc_nand_read_page(buffer,ptcout/pagesize,0,pagesize);
        jz_sfc_get_param(buffer+offset,param);
}

static char *get_chip_param_from_nand(struct nand_param_from_burner **param,int *using_way)
{
	int pagesize=0;
	char *buffer=NULL;
	int32_t nand_magic;
	pagesize=get_page_size(0,0);
	nand_magic=get_nand_magic(CONFIG_SPIFLASH_PART_OFFSET/pagesize,CONFIG_SPIFLASH_PART_OFFSET%pagesize);
	if(nand_magic==0x6e616e64)
	{
		*param=malloc(sizeof(struct nand_param_from_burner));
        	buffer=malloc(pagesize);
		read_spinand_param(buffer,CONFIG_SPIFLASH_PART_OFFSET,*param,pagesize);
		*using_way=0;
	}else{
		printf("warning: can't get right magic\n");
		*using_way=1;
	}
        return buffer;
}

static void tran_old_burnway(struct nand_param_from_burner **param)
{
	int i;
	*param=malloc(sizeof(struct nand_param_from_burner));
	(*param)->version=-1;
	(*param)->flash_type=1;
	(*param)->para_num=ARRAY_SIZE(jz_spi_nand_support_table);
	(*param)->addr=malloc((*param)->para_num*sizeof(struct jz_spi_support_from_burner));
	for(i=0;i<(*param)->para_num;i++){
		(((*param)->addr)[i]).id_manufactory=jz_spi_nand_support_table[i].id_manufactory*256+
			jz_spi_nand_support_table[i].id_device;
		(((*param)->addr)[i]).id_device=jz_spi_nand_support_table[i].id_device;
		memcpy((((*param)->addr)[i]).name,jz_spi_nand_support_table[i].name,32);
		(((*param)->addr)[i]).page_size=jz_spi_nand_support_table[i].page_size;
		(((*param)->addr)[i]).oobsize=jz_spi_nand_support_table[i].oobsize;
		(((*param)->addr)[i]).sector_size=jz_spi_nand_support_table[i].sector_size;
		(((*param)->addr)[i]).block_size=jz_spi_nand_support_table[i].block_size;
		(((*param)->addr)[i]).size=jz_spi_nand_support_table[i].size;
		(((*param)->addr)[i]).page_num=jz_spi_nand_support_table[i].page_num;
		(((*param)->addr)[i]).column_cmdaddr_bits=jz_spi_nand_support_table[i].column_cmdaddr_bits;
	}
	(*param)->partition_num=ARRAY_SIZE(jz_mtd_spinand_partition);
	(*param)->partition=jz_mtd_spinand_partition;

}
#define IDCODE_LEN (IDCODE_CONT_LEN + IDCODE_PART_LEN)

static int jz_sfc_nand_read_id(int id_dummy_nbits, int id_nbytes)
{
	unsigned char id_buf[32];
	unsigned int chip_id = 0;
	int mask = 0;
	int i;
	unsigned char cmd[1];
	if (id_nbytes >= 4) {
		printk("warnning : id is too long, error maybe occur!\n");
		return -1;
	}

	/* read id */
	cmd[0] = 0x9f;
	sfc_send_cmd(&cmd[0], id_nbytes, 0, 0, id_dummy_nbits, 1, 0);
	sfc_read_data(id_buf, id_nbytes);

	for(i = 0; i < (id_nbytes); i++) {
		mask |= (0xff << (i * 8));
	}
	for(i = 0; i < (id_nbytes); i++) {
		chip_id |= (id_buf[i] << ((id_nbytes - i - 1) * 8));
	}
//	printk("chip_id=%x mask=%x\n", chip_id, mask);

	return chip_id & mask;
}

int jz_sfc_nand_init(int sfc_quad_mode,struct nand_param_from_burner *param)
{
	u8 idcode[IDCODE_LEN + 1];
	struct nand_chip *chip;
	struct mtd_info *mtd;
	struct jz_spi_support_from_burner *spi_flash;
	mtd = &nand_info[0];
	int using_way;
	int i;
	int chip_id;
	int try_times = 0;
	int id_dummy_nbits;
	int id_nbytes;
	sfc_for_nand_init(sfc_quad_mode);
#ifndef CONFIG_BURNER
        char *buffer=get_chip_param_from_nand(&param,&using_way);
	if(using_way==1)		//use old way
	{
		tran_old_burnway(&param);
	}
#endif

	/* probe spi nand flash id */
	do {
		if (try_times == 0) {
			id_dummy_nbits = 0;
			id_nbytes = 3;
		} else if (try_times ==1) {
			id_dummy_nbits = 8;
			id_nbytes = 3;
		}  else {
			id_dummy_nbits = 8;
			id_nbytes = 2;
		}

		chip_id = jz_sfc_nand_read_id(id_dummy_nbits, id_nbytes);

		struct jz_spi_support_from_burner *p = param->addr;
		for (i = 0; i < param->para_num; i++) {
			if (chip_id == p->id_manufactory) {
				spi_flash = p;
				break;
			} else {
				spi_flash = NULL;
			}
			p++;
		}
		if (spi_flash) {
			break;
		}
	} while(++try_times < 3);


	if (spi_flash == NULL) {
		printf("ingenic: Unsupported ID %04x\n", (idcode[0] <<8) |idcode[1]);
		return -1;
	}

	chip = malloc(sizeof(struct nand_chip));
	if (!chip)
		return -ENOMEM;
	memset(chip,0,sizeof(struct nand_chip));

	mtd->size = spi_flash->size;
	mtd->flags |= MTD_CAP_NANDFLASH;
	mtd->erasesize = spi_flash->block_size;
	mtd->writesize = spi_flash->page_size;
	mtd->oobsize = spi_flash->oobsize;

	column_cmdaddr_bits = spi_flash->column_cmdaddr_bits;

	chip->select_chip = NULL;
	chip->badblockbits = 8;
	chip->scan_bbt = nand_default_bbt;
	chip->block_bad = jz_sfcnand_block_bad_check;
	chip->block_markbad = jz_sfcnand_block_markbad;
	chip->ecc.layout= &gd5f_ecc_layout_128; // for erase ops
	chip->bbt_erase_shift = chip->phys_erase_shift = ffs(mtd->erasesize) - 1;

	if (!(chip->options & NAND_OWN_BUFFERS))
		chip->buffers = memalign(ARCH_DMA_MINALIGN,sizeof(*chip->buffers));

	/* Set the bad block position */
	if (mtd->writesize > 512 || (chip->options & NAND_BUSWIDTH_16))
		    chip->badblockpos = NAND_LARGE_BADBLOCK_POS;
	else
		    chip->badblockpos = NAND_SMALL_BADBLOCK_POS;


	mtd->priv = chip;

	jz_sfcnand_ext_init();
	mtd_sfcnand_init(mtd);
	nand_register(0);
	return 0;
}
static int mtd_sfcnand_partition_analysis(unsigned int blk_sz,int partcount,struct jz_spinand_partition *jz_mtd_spinand_partition)
{
	char mtdparts_env[X_ENV_LENGTH];
	char command[X_COMMAND_LENGTH];
	int ptcount = partcount;
	int part, ret;

	memset(mtdparts_env, 0, X_ENV_LENGTH);
	memset(command, 0, X_COMMAND_LENGTH);

	/*MTD part*/
	sprintf(mtdparts_env, "mtdparts=nand:");
	for (part = 0; part < ptcount; part++) {
		if (jz_mtd_spinand_partition[part].size == -1) {
			sprintf(mtdparts_env,"%s-(%s)", mtdparts_env,
					jz_mtd_spinand_partition[part].name);
			break;
		} else if (jz_mtd_spinand_partition[part].size  != 0) {
			if(jz_mtd_spinand_partition[part].size % blk_sz != 0)
				printf("ERROR:the partition [%s] don't algin as block size [0x%08x] ,it will be error !\n",jz_mtd_spinand_partition[part].name,blk_sz);

			sprintf(mtdparts_env,"%s%dK@%d(%s),", mtdparts_env,
					jz_mtd_spinand_partition[part].size / 0x400,
					jz_mtd_spinand_partition[part].offset,
					jz_mtd_spinand_partition[part].name);
		} else
			break;
	}
	debug("env:mtdparts=%s\n", mtdparts_env);
	setenv("mtdids", mtdids_default);
	setenv("mtdparts", mtdparts_env);
	setenv("partition", NULL);
}
extern struct jz_spinand_partition *get_partion_index(u32 startaddr,u32 length,int *pt_index);

int mtd_sfcnand_probe_burner(/*MTDPartitionInfo *pinfo,*/int *erase_mode,int sfc_quad_mode,struct nand_param_from_burner *param)
{
	int ret;
	struct mtd_info *mtd;
	mtd = &nand_info[0];
	struct nand_chip *chip;
	unsigned int i=0;
	//int wppin = pinfo->gpio_wp;

	ret = jz_sfc_nand_init(sfc_quad_mode,param);

	/*0: none 1, force-erase*/
	if (*erase_mode == 1)
		ret = run_command("nand scrub.chip -y", 0);

	chip = mtd->priv;
	chip->scan_bbt(mtd);
	chip->options |= NAND_BBT_SCANNED;
	mtd_sfcnand_partition_analysis(mtd->erasesize,param->partition_num,param->partition);
	return 0;
}
