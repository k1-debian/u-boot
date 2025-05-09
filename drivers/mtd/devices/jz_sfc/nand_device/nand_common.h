#ifndef __NAND_COMMON_H
#define __NAND_COMMON_H
#include <linux/types.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spinand.h>

#define DEVICE_ID_STRUCT(id, name_string, parameter) {      \
                .id_device = id,          \
                .name = name_string,                      \
		.param = parameter,			    \
}

void nand_pageread_to_cache(struct sfc_transfer *transfer, struct flash_operation_message *op_info);

void nand_single_read(struct sfc_transfer *transfer, struct flash_operation_message *op_info);

void nand_quad_read(struct sfc_transfer *transfer, struct flash_operation_message *op_info);

void nand_write_enable(struct sfc_transfer *transfer, struct flash_operation_message *op_info);


void nand_single_load(struct sfc_transfer *transfer, struct flash_operation_message *op_info);

void nand_quad_load(struct sfc_transfer *transfer, struct flash_operation_message *op_info);

void nand_program_exec(struct sfc_transfer *transfer, struct flash_operation_message *op_info);

int32_t nand_get_program_feature(struct flash_operation_message *op_info);

void nand_block_erase(struct sfc_transfer *transfer, struct flash_operation_message *op_info);

int32_t nand_get_erase_feature(struct flash_operation_message *op_info);

void nand_set_feature(struct sfc_transfer *transfer, uint8_t addr, uint32_t *val);

void nand_get_feature(struct sfc_transfer *transfer, uint8_t addr, uint8_t *val);

int32_t nand_get_ecc_conf(struct sfc_flash *flash, uint8_t addr);
#endif
