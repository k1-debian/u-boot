#ifdef CONFIG_MTD_SFCNOR
#include <asm/arch/sfc.h>
#include <asm/arch/spinor.h>

extern struct debug_param *debug_args;
extern struct nor_partition *get_partition_index(u32 offset,u32 length,int *pt_index);

extern struct burner_params params;
extern struct mini_spi_nor_info mini_params;
extern struct legacy_params *params_compatibility();
#ifdef CONFIG_SFC_NOR_CONCAT
extern int sfc_nor_read_chip_id(uint32_t chip_index, uint32_t *jedec_id);
#else
extern int sfc_nor_flash_init(void);
extern unsigned int get_norflash_id(void);
#endif

static void sfcnor_patch_boot_image_common(unsigned char *buf)
{
	if (ddr_args != NULL && ddr_args->ddr_type > 0)
		*(volatile unsigned int *)(buf + 128) = ddr_args->ddr_type;

	if (*(volatile unsigned int *)(buf + 512) == 0 ||
	    *(volatile unsigned int *)(buf + 512) > 65535)
		*(volatile unsigned int *)(buf + 512) = 0x1111;
}

int sfc_reset()
{
	return sfc_nor_reset();
}

int sfcnor_get_flash_id(uint32_t chip_index, uint32_t *jedec_id)
{
	if (!jedec_id)
		return -EINVAL;

#ifdef CONFIG_SFC_NOR_CONCAT
	return sfc_nor_read_chip_id(chip_index, jedec_id);
#else
	if (chip_index)
		return -EINVAL;
	if (sfc_nor_flash_init() < 0) {
		LOG_ERROR("sfc nor init failed\n");
		return -EIO;
	}

	*jedec_id = get_norflash_id();
	return 0;
#endif
}

static void sfcnor_add_info_to_flash(unsigned char *buf)
{
	struct legacy_params *l_params;
	int spl_version;
	uint32_t param_offset = CONFIG_SPIFLASH_PART_OFFSET;
	/* spl_version is in 16byte of spl header,
	 * spl_version = 0x01, spl is new code, NOR_VERSION is 2,
	 * spl_version = 0x00, spl is old code, NOR_VERSION is 1.
	 * */

	if ((int)(spi_args->param_offset) > 0)
		param_offset = spi_args->param_offset;

	spl_version = buf[CONFIG_SPL_VERSION_OFFSET];
	switch (spl_version) {
		case 0:
			l_params = params_compatibility();
			memcpy(buf + param_offset, l_params, sizeof(struct legacy_params));
			break;
		case 1:
			params.version = NOR_VERSION;
			memset(buf + param_offset, 0, sizeof(struct spiflash_info));
			memcpy(buf + param_offset, &params, sizeof(struct burner_params));
			memcpy(buf + param_offset + sizeof(struct burner_params),
			       &mini_params, sizeof(struct mini_spi_nor_info));
			break;
		default:
			LOG_ERROR("spl uboot version error !\n");
			break;
	}

	sfcnor_patch_boot_image_common(buf);
}

int sfcnor_read(struct cloner *cloner)
{
	int ret = 0;
	u32 addr = cloner->cmd->read.offset;
	u32 len = cloner->read_req->length;
	void *buf = (void *)cloner->read_req->buf;

	ret = sfc_nor_read(addr, len, buf);
	if(ret < 0)
		LOG_ERROR("%s error\n",__func__);

	return ret;
}

int sfc_nor_program(struct cloner *cloner)
{
	u32 offset = cloner->cmd->write.partition + cloner->cmd->write.offset;
	u32 length = cloner->cmd->write.length;
	int blk_size = spi_args->spi_erase_block_size;
	void *addr = (void *)cloner->write_req->buf;
	int ret = 0;
	int len = 0;
	struct nor_partition *partition;
	int pt_index;
	static int pt_index_bak = -1;

	LOG_INFO("the offset = %x\n",offset);

	if (length < blk_size || length%blk_size == 0){
		len = length;
		LOG_INFO("the length = %x\n",length);
	}
	else{
		len = (length/blk_size)*blk_size + blk_size;
		LOG_INFO("the length = %x, is no enough %x\n",len,blk_size);
	}

	partition = get_sfc_nor_partition(offset,len, &pt_index);

	if(pt_index < 0 || partition == NULL){
		LOG_ERROR("out of partition\n");
		return -EIO;
	}

	if (spi_args->spi_erase == PART_ERASE || partition->mask_flags == PART_RO) {
		if (partition->manager_mode == MTD_D_MODE)
			pt_index = offset / blk_size;
		if(pt_index != pt_index_bak){
			pt_index_bak = pt_index;

			if (partition->manager_mode == MTD_D_MODE) {
				ret = sfc_nor_erase(offset, len);
				LOG_INFO("SF: %zu bytes @ %#x Erased: %s\n",
						(size_t)len, (u32)offset,
						ret ? "ERROR" : "OK");
			} else {
				ret = sfc_nor_erase(partition->offset, partition->size);
				LOG_INFO("SF: %zu bytes @ %#x Erased: %s\n",
						(size_t)partition->size, (u32)partition->offset,
						ret ? "ERROR" : "OK");
			}
		}
	}

	if (offset == 0 && spi_args->download_params != 0) {
#if defined(CONFIG_SFC_NOR_CONCAT) && defined(CONFIG_BURNER)
		ret = sfc_nor_concat_inject_burner_params(offset, len, addr);
		if (ret < 0)
			return ret;
		if (ret > 0)
			sfcnor_patch_boot_image_common(addr);
		else
#endif
			sfcnor_add_info_to_flash(addr);
	}

	ret = sfc_nor_write(offset, len, addr);
	LOG_INFO("SF: %zu bytes @ %#x write: %s\n", (size_t)len, (u32)offset,
			ret ? "ERROR" : "OK");

	if(debug_args->write_back_chk){
		if(!readbuf){
			readbuf = malloc(len);
			if (!readbuf) {
				LOG_ERROR("malloc read buffer spaces error!\n");
				return -1;
			}
		}
		memset(readbuf,0,len);
		ret = sfc_nor_read(offset,len,readbuf);
		if(ret){
			LOG_INFO("SF: write back check read  ops error,please check flash info !\n");
			return -1;
		}
		ret = buf_compare(cloner->write_req->buf,readbuf,len,offset);
		LOG_INFO("SF: %zu bytes @ %#x check: %s\n", (size_t)len, (u32)offset, ret ? "ERROR" : "OK");
	}
	return ret;
}

#endif /*CONFIG_MTD_SFCNOR*/
