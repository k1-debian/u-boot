/*
 * Ingenic JZ SFC driver
 *
 * Copyright (c) 2013 Ingenic Semiconductor Co.,Ltd
 * Author: Tiger <xyfu@ingenic.cn>
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

#include <config.h>
#include <common.h>
#include <spi.h>
#include <spi_flash.h>
#include <malloc.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>
#include <asm/arch/spi.h>
#include <asm/arch/sfc-jz.h>
#include <asm/arch/clk.h>
#include <asm/arch/base.h>
#include <malloc.h>
#include <errno.h>

#include "jz_spi.h"
#include "jz_sf_internal.h"

static struct jz_spi_support gparams;
static struct nor_sharing_params pdata;

struct spi_quad_mode *quad_mode = NULL;
/* wait time before read status (us) for spi nand */
//static int t_reset = 500;
int mode = 0;
int flag = 0;
int sfc_is_init = 0;
unsigned int sfc_rate = 0;
unsigned int sfc_quad_mode = 0;
unsigned int quad_mode_is_set = 0;
unsigned int burner_read_id = 0;

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
void sfc_cmd_en(int channel, unsigned int value)
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
void sfc_dev_addr_dummy_bytes(int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~TRAN_CONF_DMYBITS_MSK;
	tmp |= (value << TRAN_CONF_DMYBITS_OFFSET);
	jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
}

void sfc_set_mode(int channel, int value)
{
	unsigned int tmp;
	tmp = jz_sfc_readl(SFC_TRAN_CONF(channel));
	tmp &= ~(TRAN_MODE_MSK);
	tmp |= (value << TRAN_MODE_OFFSET);
	jz_sfc_writel(tmp,SFC_TRAN_CONF(channel));
}

static void sfc_set_quad_mode()
{
	/* the paraterms is
	 * cmd , len, addr,addr_len
	 * dummy_byte, daten
	 * dir
	 *
	 * */
	unsigned char cmd[5];
	unsigned int buf = 0;
	unsigned int tmp = 0;
	int i = 10;

	if(quad_mode != NULL){
		cmd[0] = CMD_WREN;
		cmd[1] = quad_mode->WRSR_CMD;
		cmd[2] = quad_mode->RDSR_CMD;
		cmd[3] = CMD_RDSR;

		sfc_send_cmd(&cmd[0],0,0,0,0,0,1);

		sfc_send_cmd(&cmd[1],quad_mode->WD_DATE_SIZE,0,0,0,1,1);
		sfc_write_data(&quad_mode->WRSR_DATE,1);

		sfc_send_cmd(&cmd[3],1,0,0,0,1,0);
		sfc_read_data(&tmp, 1);

		while(tmp & CMD_SR_WIP) {
			sfc_send_cmd(&cmd[3],1,0,0,0,1,0);
			sfc_read_data(&tmp, 1);
		}

		sfc_send_cmd(&cmd[2], quad_mode->RD_DATE_SIZE,0,0,0,1,0);
		sfc_read_data(&buf, 1);
		while(!(buf & quad_mode->RDSR_DATE)&&((i--) > 0)) {
			sfc_send_cmd(&cmd[2], quad_mode->RD_DATE_SIZE,0,0,0,1,0);
			sfc_read_data(&buf, 1);
		}

		quad_mode_is_set = 1;
		printf("set quad mode is enable.the buf = %x\n",buf);
	}else{

		printf("the quad_mode is NULL,the nor flash id we not support\n");
	}
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

int jz_sfc_chip_erase_2()
{

	/* the paraterms is
	 * cmd , len, addr,addr_len
	 * dummy_byte, daten
	 * dir
	 *
	 * */

	unsigned char cmd[6];
	cmd[0] = CMD_WREN;
	cmd[1] = CMD_ERASE_CE;
	cmd[5] = CMD_RDSR;
	unsigned int  buf = 0;
	int err = 0;

	if(sfc_is_init == 0){
		err = sfc_init();
		if(err < 0){
			printf("the quad mode is not support\n");
			return -1;
		}
	}

	if(sfc_quad_mode == 1){
		if(quad_mode_is_set == 0){
			sfc_set_quad_mode();
		}
	}

	jz_sfc_writel(1 << 2,SFC_TRIG);
	sfc_send_cmd(&cmd[0],0,0,0,0,0,1);

	sfc_send_cmd(&cmd[1],0,0,0,0,0,1);

	sfc_send_cmd(&cmd[5], 1,0,0,0,1,0);
	sfc_read_data(&buf, 1);
	printf("sfc start chip erase\n");
	while(buf & CMD_SR_WIP) {
		sfc_send_cmd(&cmd[5], 1,0,0,0,1,0);
		sfc_read_data(&buf, 1);
	}
	printf("########## chip erase ok ######### \n");
	return 0;
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


int read_sfcnand_id(u8 *response,size_t len)
{
	/* the paraterms is
	* cmd , len, addr,addr_len
	* dummy_byte, daten
	* dir
	*
	* */
	unsigned char cmd[1];
	//  unsigned char chip_id[4];
	unsigned int chip_id = 0;
	cmd[0] = CMD_RDID;
	sfc_send_cmd(&cmd[0],len,0,1,0,1,0);
	sfc_read_data(response,len);
	printf("id0=%02x\n",response[0]);
	printf("id1=%02x\n",response[1]);
	printf("SFC_DEV_STA_RT=0x%08x,\n",jz_sfc_readl(SFC_DEV_STA_RT));
	//  *idcode = chip_id[0];
}

void sfc_nor_RDID(unsigned int *idcode)
{
	/* the paraterms is
	 * cmd , len, addr,addr_len
	 * dummy_byte, daten
	 * dir
	 *
	 * */
	unsigned char cmd[1];
//	unsigned char chip_id[4];
	unsigned int chip_id = 0;

	cmd[0] = CMD_RDID;
	sfc_send_cmd(&cmd[0],3,0,0,0,1,0);
	sfc_read_data(&chip_id, 1);
//	*idcode = chip_id[0];
	*idcode = chip_id & 0x00ffffff;
}

#ifdef CONFIG_BURNER
static unsigned int jz_nor_reset()
{
	unsigned char cmd[2]={0x66, 0x99};

	sfc_send_cmd(&cmd[0],0,0,0,0,0,1);
	sfc_send_cmd(&cmd[1],0,0,0,0,0,1);
	udelay(60);
}

int get_norflash_params_from_burner(unsigned char *addr)
{
	unsigned int idcode,chipnum,i;
	struct norflash_params *tmp;
	struct spi_flash flash;

	unsigned int flash_type = *(unsigned int *)(addr + 4);	//0:nor 1:nand
	if(flash_type == 0){

		burner_read_id = 1;
		if(sfc_is_init == 0){
			sfc_init();
		}
		burner_read_id = 0;

		jz_nor_reset();
		sfc_nor_RDID(&idcode);
		printf("the norflash chip_id is is %x\n",idcode);

		pdata.magic = NOR_MAGIC;
		pdata.version = CONFIG_NOR_VERSION;

		chipnum = *(unsigned int *)(addr + 8);
		for(i = 0; i < chipnum; i++){
			tmp = (struct norflash_params *)(addr + 12 +sizeof(struct norflash_params) * i);
			if(tmp->id == idcode) {
				memcpy(&pdata.norflash_params,tmp,sizeof(struct norflash_params));
				printf("-----break,break,break,break %x\n",idcode);
				break;
			}
		}

		if(i >= chipnum){
			printf("none norflash support for the table ,please check the burner norflash supprot table\n");
			tmp = (struct norflash_params *)malloc(sizeof(struct norflash_params));

			sfc_flash_scan(&flash,idcode,tmp);
			memcpy(&pdata.norflash_params,tmp,sizeof(struct norflash_params));
			free(tmp);
		}

		pdata.norflash_partitions.num_partition_info = *(unsigned int *)(addr + 12 + sizeof(struct norflash_params) * chipnum);
		memcpy(&pdata.norflash_partitions.nor_partition[0] ,addr + 12 +sizeof(struct norflash_params) * chipnum + 4, \
				sizeof(struct nor_partition) * pdata.norflash_partitions.num_partition_info);
	}else{
		printf("the params recive from cloner not't the norflash\n");
		return -1;
	}
	return 0;
}

static void write_norflash_params_to_spl(unsigned int addr)
{
	memcpy(addr+CONFIG_SPIFLASH_PART_OFFSET,&pdata,sizeof(struct nor_sharing_params));
}
#if 0
unsigned int get_partition_index(u32 offset,u32 length, int *pt_offset, int *pt_size)
{
	int i;

	for(i = 0; i < pdata.norflash_partitions.num_partition_info; i++){
		if(offset >= pdata.norflash_partitions.nor_partition[i].offset && \
				(offset + length) <= (pdata.norflash_partitions.nor_partition[i].offset + \
				pdata.norflash_partitions.nor_partition[i].size)){
			*pt_offset = pdata.norflash_partitions.nor_partition[i].offset;
			*pt_size = pdata.norflash_partitions.nor_partition[i].size;
			break;
		}else if(offset >= pdata.norflash_partitions.nor_partition[i].offset && \
				offset < (pdata.norflash_params.chipsize) && \
				(pdata.norflash_partitions.nor_partition[i].size == 0xffffffff)){ /*size == -1*/
			*pt_offset = pdata.norflash_partitions.nor_partition[i].offset;
			*pt_size = pdata.norflash_params.chipsize - pdata.norflash_partitions.nor_partition[i].offset;
			break;
		}
	}
	if(i == pdata.norflash_partitions.num_partition_info){
		*pt_offset = -1;
		*pt_size = -1;
		printf("partition size not align with write transfer size \n");
		return -1;
	}
	return i;
}

int check_offset(u32 offset,u32 length)
{
	int i;

	for(i = 1; i < pdata.norflash_partitions.num_partition_info; i++){
		if(offset < pdata.norflash_partitions.nor_partition[i].offset && \
				offset > (pdata.norflash_partitions.nor_partition[i-1].offset + \
				pdata.norflash_partitions.nor_partition[i-1].size)){
			break;
		}
	}
	if(i >= pdata.norflash_partitions.num_partition_info){
		return i;
	}else if((offset+length) > pdata.norflash_partitions.nor_partition[i].offset ){
		return -1;
	}else{
		return i;
	};
}
#endif

int sfc_flash_scan(struct spi_flash *flash,unsigned int id,struct norflash_params *norflash)
{
	struct spi_slave *spi = flash->spi;
	const struct spi_flash_params *params;
	u16 jedec, ext_jedec;
	u8 dual_flash,shift;
	u8 idcode[5],code[4];
	int ret;
	int i;

	for(i = 0; i < 5 ; i ++){
		idcode[i] = id >> (i*8);
	}

	char *p_id = (char *)&id;
	for(i = 2; i <= 4 ; i ++){
		*p_id = idcode[4-i];
		p_id ++;
	}

	jedec = idcode[1] << 8 | idcode[2];
	ext_jedec = idcode[3] << 8 | idcode[4];

	/* Validate params from spi_flash_params table */
	params = spi_flash_params_table;
	for (; params->name != NULL; params++) {
		if ((params->jedec ) == id) {
			if (params->ext_jedec == 0)
				break;
		}
	}

	if (!params->name) {
		printf("SF: Unsupported flash IDs: ");
		printf("manuf %02x, jedec %04x, ext_jedec %04x\n",
		       idcode[0], jedec, ext_jedec);
		return -EPROTONOSUPPORT;
	}

	/* Flash powers up read-only, so clear BP# bits */
	if (idcode[0] == SPI_FLASH_CFI_MFR_ATMEL ||
	    idcode[0] == SPI_FLASH_CFI_MFR_MACRONIX ||
	    idcode[0] == SPI_FLASH_CFI_MFR_SST){
		norflash->quad_mode.WRSR_CMD = CMD_WRITE_STATUS;
		norflash->quad_mode.WRSR_DATE = 0x40;
		norflash->quad_mode.WD_DATE_SIZE = 1;
		norflash->quad_mode.RD_DATE_SIZE = 1;
	}


	u8 option = 0;
	/* Assign spi data */
	flash->name = params->name;
	strcpy(norflash->name , params->name);
	dual_flash = option;

	/* Compute the flash size */
	shift = (dual_flash & SF_DUAL_PARALLEL_FLASH) ? 1 : 0;
	if (ext_jedec == 0x4d00) {
		if ((jedec == 0x0215) || (jedec == 0x216) || (jedec == 0x220)){
			flash->page_size = 256;
			norflash->pagesize = 256;
		}else{
			flash->page_size = 512;
			norflash->pagesize = 512;
		}
	} else {
		flash->page_size = 256;
		norflash->pagesize = 256;
	}
	norflash->pagesize <<= shift;
	norflash->sectorsize = params->sector_size << shift;
	flash->size = norflash->sectorsize * params->nr_sectors << shift;
	norflash->chipsize = norflash->sectorsize * params->nr_sectors << shift;
	norflash->erasesize = SPI_FLASH_ERASE_SIZE;  /*default erase size is 32K*/

	u8 erase_cmd,write_cmd;
	if (params->flags & SECT_4K) {
		erase_cmd = CMD_ERASE_4K;
		norflash->block_info.cmd_blockerase = CMD_ERASE_4K;
		norflash->block_info.blocksize = norflash->erasesize;
	} else if (params->flags & SECT_32K) {
		erase_cmd = CMD_ERASE_32K;
		norflash->block_info.cmd_blockerase = CMD_ERASE_32K;
		norflash->block_info.blocksize = norflash->erasesize;
	} else {
		erase_cmd = CMD_ERASE_64K;
		norflash->block_info.cmd_blockerase = CMD_ERASE_64K;
		norflash->block_info.blocksize = norflash->erasesize;
	}

	/*when chipsize > 16MB,addrsize is 4*/
	if(norflash->chipsize > 0x1000000)
		norflash->addrsize = 4;
	else
		norflash->addrsize = 3;

	norflash->id = id;

	if (params->flags & WR_QPP )
		write_cmd = CMD_QUAD_PAGE_PROGRAM;
	else
		write_cmd = CMD_PAGE_PROGRAM;

	switch (idcode[0])
	{
	case SPI_FLASH_CFI_MFR_MACRONIX:
		 norflash->quad_mode.RDSR_CMD = CMD_RDSR;
		 norflash->quad_mode.WRSR_CMD = CMD_WRSR;
		 norflash->quad_mode.WD_DATE_SIZE = 1;
		 norflash->quad_mode.RD_DATE_SIZE = 1;
		 norflash->quad_mode.RDSR_DATE =(1<<6);
		 norflash->quad_mode.WRSR_DATE = (1<<6);
		 norflash->quad_mode.dummy_byte = 0;
		 norflash->quad_mode.sfc_mode = 0x05;
		 norflash->quad_mode.cmd_read = CMD_QUAD_READ;
		 break;
        case SPI_FLASH_CFI_MFR_SPANSION:
        case SPI_FLASH_CFI_MFR_WINBOND:
		 norflash->quad_mode.RDSR_CMD = CMD_RDSR_1;
		 norflash->quad_mode.WRSR_CMD = CMD_WRSR;
		 norflash->quad_mode.WD_DATE_SIZE = 2;
		 norflash->quad_mode.RD_DATE_SIZE = 1;
		 norflash->quad_mode.RDSR_DATE = (1<<1);
		 norflash->quad_mode.WRSR_DATE = (1<<9);
		 norflash->quad_mode.sfc_mode = 0x06;
		 norflash->quad_mode.dummy_byte = 6;
		 norflash->quad_mode.cmd_read = CMD_READ_QUAD_IO_FAST;
		 break;
        case SPI_FLASH_CFI_MFR_STMICRO:
		 norflash->quad_mode.RDSR_CMD = 0x65;
		 norflash->quad_mode.WRSR_CMD = 0x61;
		 norflash->quad_mode.WD_DATE_SIZE = 1;
		 norflash->quad_mode.RD_DATE_SIZE = 1;
		 norflash->quad_mode.RDSR_DATE = (1<<7);
		 norflash->quad_mode.WRSR_DATE = (1<<7);
		 norflash->quad_mode.dummy_byte = 0;
		 break;
	case 0xc8:
		 norflash->quad_mode.RDSR_CMD = CMD_RDSR_1;
		 norflash->quad_mode.WRSR_CMD = CMD_WRSR_1;
		 norflash->quad_mode.WD_DATE_SIZE = 1;
		 norflash->quad_mode.RD_DATE_SIZE = 1;
		 norflash->quad_mode.RDSR_DATE =(1<<1);
		 norflash->quad_mode.WRSR_DATE = (1<<1);
		 norflash->quad_mode.sfc_mode = 0x05;
		 norflash->quad_mode.dummy_byte = 8;
		 norflash->quad_mode.cmd_read = CMD_QUAD_READ;
		 break;
	default:
           printf("SF: Need set QEB func for %02x flash\n", idcode);
           return -1;
	}

	return ret;
}
#endif


int read_sfcnand_id_func2(u8 *response,size_t len)
{
	/* =send=> [0x9F]    ; =recv=>[manu_id + dev_id] */
	unsigned char cmd[1];
	unsigned int chip_id = 0;
	cmd[0] = CMD_RDID;

	sfc_send_cmd(&cmd[0],len,0,0,0,1,0);
	sfc_read_data(response, len);
	printf("id0_2=%02x\n",response[0]);
	printf("id1_2=%02x\n",response[1]);
	printf("SFC_DEV_STA_RT=0x%08x,\n",jz_sfc_readl(SFC_DEV_STA_RT));
	return 0;
}




