#include "burn_printf.h"

#ifdef CONFIG_JZ_SFC
extern unsigned int sfc_rate;
extern unsigned int get_partition_index(u32 offset,u32 length,int *pt_offset, int *pt_size);

#define READBUF_SIZE	(512*1024)
static char *readbuf = NULL;

int sfc_erase(struct cloner *cloner)
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;
	int err = 0;
	struct spi_args *spi_arg = &cloner->args->spi_args;
	spi.rate  = spi_arg->rate;
	sfc_rate = spi_arg->sfc_rate;
	jz_sfc_chip_erase();

}

static int buf_compare(unsigned char *org_data,unsigned char *read_data,unsigned int len,unsigned int offset)
{
	unsigned int i,val = 0;
	unsigned int *buf1 = (unsigned int *)org_data;
	unsigned int *buf2 = (unsigned int *)read_data;
	for(i = 0; i < len / 4; i++)
	{
		if(buf1[i] != buf2[i]){
			printf("XXXXXXXXXX  compare error: org_data[%d] = 0x%08x read_data[%d] = 0x%08x addr= 0x%08x  len = %d\n",i,buf1[i],i,buf2[i],offset + i * 4,len);
			val = -1;
		}
	}
	return val;
}


int sfc_program(struct cloner *cloner)
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;
	u32 offset = cloner->cmd->write.partition + cloner->cmd->write.offset;
	u32 length = cloner->cmd->write.length;
	int blk_size = cloner->args->spi_erase_block_siz;
	void *addr = (void *)cloner->write_req->buf;
	struct spi_args *spi_arg = &cloner->args->spi_args;
	unsigned int ret;
	int len = 0,err = 0;
	struct spi_flash *flash;

	volatile int pt_offset;
	volatile int pt_size;
	volatile int pt_index;
	static pt_index_bak = -1;

	spi.enable = spi_arg->enable;
	spi.clk   = spi_arg->clk;
	spi.data_in  = spi_arg->data_in;
	spi.data_out  = spi_arg->data_out;
	spi.rate  = spi_arg->rate ;
	sfc_rate = spi_arg->sfc_rate;

	BURNNER_PRI("the offset = %x\n",offset);
	BURNNER_PRI("the length = %x\n",length);

	if (length < blk_size || length%blk_size == 0){
		len = length;
		BURNNER_PRI("the length = %x\n",len);
	}
	else{
		len = (length/blk_size)*blk_size + blk_size;
		BURNNER_PRI("the length = %x, is no enough %x\n",len,blk_size);
	}

	pt_index = get_partition_index(offset,len, &pt_offset, &pt_size);

	if(pt_index < 0){
		if(length < blk_size){
			BURNNER_PRI("the length = %x, is no enough %x\n",length,blk_size);
			len = length;
		}
		int index_offset = check_offset(offset,len);
		if(index_offset < 0){
			BURNNER_PRI("the offset + len is greater than the partition offset,please check it\n");
			return -1;
		}
		if (cloner->args->spi_erase == SPI_NO_ERASE) {
			ret = sfc_nor_erase(offset, len);
			BURNNER_PRI("SF: %zu bytes @ %#x Erased: %s\n", (size_t)len, (u32)offset,
				ret ? "ERROR" : "OK");
		}
		ret = sfc_nor_write(offset, len, addr);
		BURNNER_PRI("SF: %zu bytes @ %#x write: %s\n", (size_t)len, (u32)offset,
			ret ? "ERROR" : "OK");

		return ret;
	}

	if (cloner->args->spi_erase == SPI_NO_ERASE) {
		if(pt_index != pt_index_bak){
			pt_index_bak = pt_index;
			ret = sfc_nor_erase(pt_offset, pt_size);
			BURNNER_PRI("SF: %zu bytes @ %#x Erased: %s\n", (size_t)len, (u32)offset,
				ret ? "ERROR" : "OK");
		}
	}

	ret = sfc_nor_write(offset, len, addr);
	BURNNER_PRI("SF: %zu bytes @ %#x write: %s\n", (size_t)len, (u32)offset,
			ret ? "ERROR" : "OK");

	if(cloner->args->write_back_chk){
		if(!readbuf){
			readbuf = malloc(READBUF_SIZE);
			memset(readbuf,0,READBUF_SIZE);
		}
		ret = sfc_nor_read(offset,len,readbuf);
		if(ret){
			BURNNER_PRI(" write back check read  ops error,please check flash info !\n");
			return -1;
		}
		ret = buf_compare(cloner->write_req->buf,readbuf,len,offset);
		if(ret){
			return -1;
		}
	}
	return ret;
}
#endif
