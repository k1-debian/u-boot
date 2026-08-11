#ifdef CONFIG_MTD_SFCNAND
#include <common.h>
#include <nand.h>
#include <linux/mtd/mtd.h>
#include <ingenic_nand_mgr/nand_param.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spinand.h>

extern struct jz_sfcnand_partition *get_sfc_nand_partition(u32 startaddr,
							   u32 length,
							   int *pt_index);
/*******************************************************************************
 * in burner init,we find spinand information from stage2_arg
 * and change it to struct nand_param_from_burner which uboot can use
 *for chip probe,but after chip probe the struct nand_param_from_burner
 *is changed,and para_num is changed to 1,and jz_spi_support_from_burner
 *pointer addr changed to the address which param we probe.
 * ******************************************************************************/

struct jz_sfcnand_burner_param bp;
void get_burner_nandinfo()
{
	int i;
	struct jz_sfcnand_burner_param *flash_info =
		(struct jz_sfcnand_burner_param *)spi_args->flash_info;
	struct jz_sfcnand_burner_param *tmpbp = flash_info;
	const void *partition_src = &tmpbp->partition;

	bp.magic_num = tmpbp->magic_num;
	bp.partition_num= tmpbp->partition_num;

	if (bp.partition_num <= 0) {
		bp.partition = NULL;
		return;
	}

	bp.partition = malloc(sizeof(struct jz_sfcnand_partition) *
			      bp.partition_num);
	if (!bp.partition)
		return;

	memcpy(bp.partition, partition_src,
	       sizeof(struct jz_sfcnand_partition) * bp.partition_num);

#ifdef DEBUG
	struct jz_sfcnand_partition *partition = bp.partition;
	LOG_DEBUG("**** magic num = %x\n",bp.magic_num);
	LOG_DEBUG("**** partition_num = %x\n",bp.partition_num);

	for(i = 0; i < bp.partition_num; i++){
		LOG_DEBUG("name = %s\n",partition[i].name);
		LOG_DEBUG("size = %x\n",partition[i].size);
		LOG_DEBUG("offset= %x\n",partition[i].offset);
	}
#endif
}

extern nand_info_t nand_info[CONFIG_SYS_MAX_NAND_DEVICE];
static unsigned int bad_len = 0;

static int sfc_nand_skip_bad(unsigned int addr)
{
	nand_info_t *nand;
	nand = &nand_info[0];
	unsigned int offset;
	unsigned int block_size = nand->erasesize;

	offset = addr + bad_len;
	while (nand_block_isbad(nand, offset)) {
		LOG_WARNING("Skip bad block 0x%lx\n", offset);
		bad_len += block_size;
		offset += block_size;
	}
	return offset;
}

int spinand_read(struct cloner *cloner)
{
	int ret = 0;
	u32 addr = cloner->cmd->read.offset;
	u32 len = cloner->cmd->read.length;
	size_t read_len = len;
	void *buf = (void *)cloner->read_req->buf;
	nand_info_t *nand;
	nand = &nand_info[0];

	if (nand_concat_is_managed(nand)) {
		if (addr > nand->size || len > nand->size - addr) {
			LOG_ERROR("%s out of range addr=0x%x len=0x%x size=0x%llx\n",
				  __func__, addr, len,
				  (unsigned long long)nand->size);
			return -EINVAL;
		}
		ret = nand_read_skip_bad(nand, addr, &read_len, NULL,
					 nand->size - addr, buf);
		if (ret < 0)
			LOG_ERROR("%s error\n", __func__);
		return ret;
	}

	if (nand_block_isbad(nand, addr)) {
		LOG_WARNING("Skip bad block 0x%lx\n", addr);
		return 0xFF;
	}
	ret = nand_read(nand, addr, &len, buf);
	if(ret < 0)
		LOG_ERROR("%s error\n",__func__);

	return ret;
}

int sfc_nand_program(struct cloner *cloner)
{
	u32 length = cloner->cmd->write.length;
	u32 full_size = cloner->full_size;
	void *databuf = (void *)cloner->write_req->buf;
	u32 startaddr = cloner->cmd->write.partition + (cloner->cmd->write.offset);
	char command[128];
	int pt_index = -1;
	struct jz_sfcnand_partition *partition;
	int ret;

	static int pt_index_bak = -1;
	nand_info_t *nand;
	nand = &nand_info[0];
	unsigned int block_size = nand->erasesize;
	int concat = nand_concat_is_managed(nand);
        uint32_t erase_type_backup = spi_args->spi_erase;

	partition = get_sfc_nand_partition(startaddr,length,&pt_index);
	if (pt_index < 0) {
		LOG_ERROR("startaddr 0x%x can't find the pt_index or you partition size 0x%x is not align with %x\n",
                                startaddr, length, block_size);
		return -EIO;
        }

        if (spi_args->spi_erase == CHIP_ERASE && partition->mask_flags == PART_RO)
                spi_args->spi_erase = PART_ERASE;

	if (startaddr==0 && spi_args->download_params != 0) {
		sfcnand_add_info_to_flash(databuf);
	}

	if ((partition->manager_mode == MTD_MODE) || (partition->manager_mode == MTD_D_MODE)) {
		if (pt_index != pt_index_bak) {
			bad_len = 0;
		}
		if (concat)
			startaddr += bad_len;
		else
			startaddr = sfc_nand_skip_bad(startaddr);
		if (spi_args->spi_erase == PART_ERASE || partition->mask_flags == PART_RO) {
			if (pt_index != pt_index_bak ||
			    (!concat && partition->manager_mode == MTD_D_MODE &&
			     !(startaddr % block_size))) {
				memset(command, 0 , 128);
				if (!concat && partition->manager_mode == MTD_D_MODE)
					sprintf(command, "nand erase 0x%x 0x%x", startaddr, ALIGN(length, block_size));
				else
					sprintf(command, "nand erase 0x%x 0x%x", partition->offset, partition->size);
				LOG_INFO("%s\n", command);
				ret = run_command(command, 0);
				if (ret)
					goto out;
			}
		}

		if (pt_index != pt_index_bak) {
			pt_index_bak = pt_index;
		}
		if ((startaddr + length) <= (partition->size + partition->offset)) {
			size_t request_len = length;
			size_t write_len = length;
			size_t actual = 0;

			if (concat) {
				ret = nand_write_skip_bad(
					nand, startaddr, &write_len, &actual,
					partition->offset + partition->size -
					startaddr, databuf, 0);
				if (!ret && actual > request_len)
					bad_len += actual - request_len;
				length = write_len;
			} else {
				ret = nand_write(nand, startaddr, &length,
						 databuf);
			}
			LOG_INFO("nand write to offset 0x%lx, length = 0x%lx : ", startaddr, length);
			if (ret || (length == 0)) {
				LOG_ERROR("ERROR\n");
				LOG_ERROR("nand write error!\n");
				return -EIO;
			} else {
				LOG_INFO("OK\n");
			}
		} else {
			LOG_ERROR("out of partition!\n");
		}

	} else if (partition->manager_mode == UBI_MANAGER) {
		if (startaddr == partition->offset) {
			if (spi_args->spi_erase == PART_ERASE || partition->mask_flags == PART_RO) {
				if (pt_index != pt_index_bak) {
					pt_index_bak = pt_index;
					memset(command, 0 , 128);
					sprintf(command, "nand erase 0x%x 0x%x", partition->offset, partition->size);
					LOG_INFO("%s\n", command);
					ret = run_command(command, 0);
					if (ret)
						goto out;
				}
			}

			memset(command, 0, 128);
			sprintf(command, "ubi part %s", partition->name);
			LOG_INFO("%s\n", command);
			ret = run_command(command, 0);
			if (ret) {
				LOG_ERROR("ubi part error...\n");
				return ret;
			}

			memset(command, 0, X_COMMAND_LENGTH);
			sprintf(command, "ubi create %s",partition->name);
			LOG_INFO("%s\n", command);
			ret = run_command(command, 0);
			if (ret) {
				LOG_ERROR("ubi create error...\n");
				return ret;
			}
		}

		memset(command, 0, 128);
		static wlen = 0;
		wlen += length;
		if (full_size && (full_size <= length)) {
			length = full_size;
			sprintf(command, "ubi write 0x%x %s 0x%x", (unsigned)databuf, partition->name, length);
		} else if (full_size) {
			sprintf(command, "ubi write.part 0x%x %s 0x%x 0x%x",(unsigned)databuf, partition->name, length, full_size);
		} else {
			sprintf(command, "ubi write.part 0x%x %s 0x%x",(unsigned)databuf, partition->name, length);
		}


		ret = run_command(command, 0);
		if (ret) {
			LOG_ERROR("...error\n");
			return ret;
		}
	}
	if (cloner->full_size)
		cloner->full_size = 0;

        spi_args->spi_erase = erase_type_backup;

	return 0;
out:
	LOG_ERROR("...error\n");
	return ret;

}
/****************************************************************************************
 * copy spinand information from burner to u-boot-with-spl.bin
 * char *buf:u-boot-with-spl.bin data pointer
 * in function :
 * param is global variable of struct nand_param_from_burner this struct is information in spinand
 * **************************************************************************************/
void sfcnand_add_info_to_flash(char *buf)
{
        struct mtd_info *mtd;
	uint32_t param_offset = CONFIG_SPIFLASH_PART_OFFSET;

	if ((int)(spi_args->param_offset) > 0)
		param_offset = spi_args->param_offset;

	memcpy(buf + param_offset, &bp, sizeof(struct jz_sfcnand_burner_param) - 4);
	memcpy(buf + param_offset + sizeof(struct jz_sfcnand_burner_param) - 4, bp.partition, sizeof(struct jz_sfcnand_partition) * bp.partition_num);

	if(ddr_args != NULL && ddr_args->ddr_type > 0)
		*(volatile unsigned int *)(buf + 128) = ddr_args->ddr_type;

        if(spi_args->reserve_space) { 
                mtd = &nand_info[0];
		*(volatile unsigned int *)(buf + 132) = mtd->size + 0x300000;
        }

	if(*(volatile unsigned int *)(buf + 512) == 0 || *(volatile unsigned int *)(buf + 512) > 65535)
		*(volatile unsigned int *)(buf + 512) = 0x1111;
}

#endif /*CONFIG_MTD_SFCNAND*/
