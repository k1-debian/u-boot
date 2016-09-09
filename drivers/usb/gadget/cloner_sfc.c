#include "burn_printf.h"

#ifdef CONFIG_JZ_SFC
extern unsigned int sfc_rate;
extern unsigned int sfc_quad_mode;
extern int sfc_is_init;
extern unsigned int get_partition_index(u32 offset,int *pt_offset, int *pt_size);

int sfc_erase(struct cloner *cloner)
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;
	int err = 0;
	struct spi_args *spi_arg = &cloner->args->spi_args;
	sfc_quad_mode = spi_arg->sfc_quad_mode;
	spi.rate  = spi_arg->rate;
	sfc_rate = spi_arg->sfc_rate;

	if(sfc_is_init == 0){
		err = sfc_init();
		if(err < 0){
			printf("!!!!!!!!!!!!!!!!!%d,%s,the sfc init failed\n",__LINE__,__func__);
			return -1;
		}
	}
	jz_sfc_chip_erase();
	printf("sfc chip erase ok\n");

}
int sfc_program(struct cloner *cloner)
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;
	u32 offset = cloner->cmd->write.partation + cloner->cmd->write.offset;
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
	sfc_quad_mode = spi_arg->sfc_quad_mode;
	spi.rate  = spi_arg->rate ;
	sfc_rate = spi_arg->sfc_rate;

	if(sfc_is_init == 0){
		BURNNER_PRI("in sfc init\n");
		err = sfc_init();
		if(err < 0){
			printf("%d,%s,!!!!!!!!!!!!!!!!!!!!!the sfc init failed\n",__LINE__,__func__);
			return -1;
		}
	}

	BURNNER_PRI("the offset = %x\n",offset);
	BURNNER_PRI("the length = %x\n",length);

	if (length%blk_size == 0){
		len = length;
		BURNNER_PRI("the length = %x\n",len);
	}else{
		BURNNER_PRI("the length = %x, is no enough %x\n",length,blk_size);
		len = (length/blk_size)*blk_size + blk_size;
	}

	pt_index = get_partition_index(offset, &pt_offset, &pt_size);

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
		ret = sfc_nor_write(offset, len, addr,0);
		BURNNER_PRI("SF: %zu bytes @ %#x write: %s\n", (size_t)len, (u32)offset,
			ret ? "ERROR" : "OK");

		return 0;
	}

	if (cloner->args->spi_erase == SPI_NO_ERASE) {
		if(pt_index != pt_index_bak){
			pt_index_bak = pt_index;
			ret = sfc_nor_erase(pt_offset, pt_size);
			BURNNER_PRI("SF: %zu bytes @ %#x Erased: %s\n", (size_t)len, (u32)offset,
				ret ? "ERROR" : "OK");
		}
	}

	ret = sfc_nor_write(offset, len, addr,0);
	BURNNER_PRI("SF: %zu bytes @ %#x write: %s\n", (size_t)len, (u32)offset,
			ret ? "ERROR" : "OK");

	return 0;
}
#endif
