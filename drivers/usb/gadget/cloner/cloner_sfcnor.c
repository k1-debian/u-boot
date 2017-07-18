#include <asm/arch/sfc_params.h>

#ifdef CONFIG_JZ_SFC
extern struct debug_param *debug_args;

extern unsigned int sfc_rate;
extern struct nor_partition *get_partition_index(u32 offset,u32 length,int *pt_index);

static char *readbuf = NULL;

int sfc_erase()
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;
	int err = 0;
	spi.rate  = spi_args->rate;
	sfc_rate = spi_args->sfc_rate;
	int ret = jz_sfc_chip_erase();
	if (ret < 0)
		printf("sfc chip erese failed!\n");
	else
		printf("sfc chip erase ok\n");
	return ret;
}

int sfc_program(struct cloner *cloner)
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;
	u32 offset = cloner->cmd->write.partition + cloner->cmd->write.offset;
	u32 length = cloner->cmd->write.length;
	int blk_size = spi_args->spi_erase_block_siz;
	void *addr = (void *)cloner->write_req->buf;
	unsigned int ret;
	int len = 0,err = 0;
	struct spi_flash *flash;
	struct nor_partition *partition;

	volatile int pt_offset;
	volatile int pt_size;
	volatile int pt_index;
	static pt_index_bak = -1;

	spi.enable = spi_args->enable;
	spi.clk   = spi_args->clk;
	spi.data_in  = spi_args->data_in;
	spi.data_out  = spi_args->data_out;
	spi.rate  = spi_args->rate ;
	sfc_rate = spi_args->sfc_rate;

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

	partition = get_partition_index(offset,len, &pt_index);

	if(pt_index < 0 || partition == NULL){
		printf("out of partition\n");
		return -EIO;
	}

	if (spi_args->spi_erase == SPI_NO_ERASE) {
		if (partition->manager_mode == MTD_D_MODE)
			pt_index = offset / blk_size;
		if(pt_index != pt_index_bak){
			pt_index_bak = pt_index;

			if (partition->manager_mode == MTD_D_MODE) {
				ret = sfc_nor_erase(offset, length);
			} else {
				ret = sfc_nor_erase(partition->offset, partition->size);
			}
			BURNNER_PRI("SF: %zu bytes @ %#x Erased: %s\n", (size_t)len, (u32)offset,
					ret ? "ERROR" : "OK");
		}
	}

	ret = sfc_nor_write(offset, len, addr);
	BURNNER_PRI("SF: %zu bytes @ %#x write: %s\n", (size_t)len, (u32)offset,
			ret ? "ERROR" : "OK");

	if(debug_args->write_back_chk){
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
