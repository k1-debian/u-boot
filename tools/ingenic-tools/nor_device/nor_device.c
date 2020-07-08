#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asm/arch/spinor.h>


/* GD25Q127C */

void spi_nor_info_init(struct burner_params *params)
{
	struct spi_nor_info *info = &params->spi_nor_info;

	memcpy((void *)info->name, "GD25Q127C", strlen("GD25Q127C"));
	info->id = 0xc84018;

	info->read_standard.cmd = 0x03;
	info->read_standard.dummy_byte = 0;
	info->read_standard.addr_nbyte = 3;
	info->read_standard.transfer_mode = 0;

	info->read_quad.cmd = 0x6b;
	info->read_quad.dummy_byte = 8;
	info->read_quad.addr_nbyte = 3;
	info->read_quad.transfer_mode = 5;

	info->write_standard.cmd = 0x02;
	info->write_standard.dummy_byte = 0;
	info->write_standard.addr_nbyte = 3;
	info->write_standard.transfer_mode = 0;

	info->write_quad.cmd = 0x32;
	info->write_quad.dummy_byte = 0;
	info->write_quad.addr_nbyte = 3;
	info->write_quad.transfer_mode = 5;

	info->sector_erase.cmd = 0x52;
	info->sector_erase.dummy_byte = 0;
	info->sector_erase.addr_nbyte = 3;
	info->sector_erase.transfer_mode = 0;

	info->wr_en.cmd = 0x06;
	info->wr_en.dummy_byte = 0;
	info->wr_en.addr_nbyte = 0;
	info->wr_en.transfer_mode = 0;

	info->en4byte.cmd = 0;
	info->en4byte.dummy_byte = 0;
	info->en4byte.addr_nbyte = 0;
	info->en4byte.transfer_mode = 0;

	info->quad_set.cmd = 0x31;
	info->quad_set.bit_shift = 1;
	info->quad_set.mask = 1;
	info->quad_set.val = 1;
	info->quad_set.len = 1;
	info->quad_set.dummy = 0;

	info->quad_get.cmd = 0x35;
	info->quad_get.bit_shift = 1;
	info->quad_get.mask = 1;
	info->quad_get.val = 1;
	info->quad_get.len = 1;
	info->quad_get.dummy = 0;

	info->busy.cmd = 0x05;
	info->busy.bit_shift = 0;
	info->busy.mask = 1;
	info->busy.val = 0;
	info->busy.len = 1;
	info->busy.dummy = 0;

	info->tCHSH = 5;
	info->tSLCH = 5;
	info->tSHSL_RD = 20;
	info->tSHSL_WR = 50;

	info->chip_size = 16777216;
	info->page_size = 256;
	info->erase_size = 32768;

	info->quad_ops_mode = 1;
	info->addr_ops_mode = 0;
}

void mini_spi_nor_info_init(struct burner_params *params, struct mini_spi_nor_info *mini)
{
	struct spi_nor_info *info = &params->spi_nor_info;

	memcpy((void *)mini->name, (void *)info->name, sizeof(mini->name));
	mini->id = info->id;
	mini->read_standard = info->read_standard;
	mini->read_quad = info->read_quad;
	mini->wr_en = info->wr_en;
	mini->en4byte = info->en4byte;
	mini->quad_set = info->quad_set;
	mini->quad_get = info->quad_get;
	mini->busy = info->busy;
	mini->quad_ops_mode = info->quad_ops_mode;
	mini->chip_size = info->chip_size;
	mini->page_size = info->page_size;
	mini->erase_size = info->erase_size;
}

void norflash_partitions_init(struct burner_params *params)
{
	struct norflash_partitions *partitions = &params->norflash_partitions;
	struct nor_partition *nor_partition = partitions->nor_partition;

	/* max 10 partitions*/
	partitions->num_partition_info = 3;

	memcpy(nor_partition[0].name, "uboot", strlen("uboot"));
	nor_partition[0].offset = 0x0;
	nor_partition[0].size =   0x40000;
	nor_partition[0].mask_flags = NORFLASH_PART_RW;

	memcpy(nor_partition[1].name, "kernel", strlen("kernel"));
	nor_partition[1].offset = 0x40000;
	nor_partition[1].size =   0x300000;
	nor_partition[1].mask_flags = NORFLASH_PART_RW;

	memcpy(nor_partition[2].name, "rootfs", strlen("rootfs"));
	nor_partition[2].offset = 0x360000;
	nor_partition[2].size = 0xca0000;
	nor_partition[2].mask_flags = NORFLASH_PART_RW;
}

int nor_device_init(struct builtin_params *builtin_params)
{
	struct burner_params *params = &builtin_params->burner_params;
	struct mini_spi_nor_info *mini = &builtin_params->mini_spi_nor_info;

	/* 1.other params */
	params->magic = NOR_MAGIC;
	params->version = NOR_VERSION;
	params->fs_erase_size = 32768;
	params->uk_quad = 1;

	/* 2.spi nor info params */
	spi_nor_info_init(params);

	/* 3.partitions params */
	norflash_partitions_init(params);
	mini_spi_nor_info_init(params, mini);

	/* 4.mini spi nor info params */
	if(!params->spi_nor_info.id && !mini->id) {
		printf("nor builtin params init fail!\n");
		return -EINVAL;
	}

	return 0;
}

