#ifndef __NAND_COMMON_H
#define __NAND_COMMON_H

void nand_pageread_to_cache(struct sfc_transfer *transfer, struct cmd_info *cmd, uint32_t pageaddr);

void nand_single_read(struct sfc_transfer *transfer, struct cmd_info *cmd,
			uint32_t columnaddr, uint8_t columnlen, void *buffer, uint32_t len);

void nand_quad_read(struct sfc_transfer *transfer, struct cmd_info *cmd,
			    uint32_t columnaddr, uint8_t columnlen, void *buffer, uint32_t len);

void nand_write_enable(struct sfc_transfer *transfer, struct cmd_info *cmd);


void nand_single_load(struct sfc_transfer *transfer, struct cmd_info *cmd, uint32_t columnaddr, void *buffer, uint32_t len);

void nand_quad_load(struct sfc_transfer *transfer, struct cmd_info *cmd, uint32_t columnaddr, void *buffer, uint32_t len);

void nand_program_exec(struct sfc_transfer *transfer, struct cmd_info *cmd, uint32_t pageaddr);

int32_t nand_get_program_feature(struct sfc_flash *flash);

void nand_block_erase(struct sfc_transfer *transfer, struct cmd_info *cmd, uint32_t pageaddr);

int32_t nand_get_erase_feature(struct sfc_flash *flash);

void nand_set_feature(struct sfc_transfer *transfer, struct cmd_info *cmd, uint8_t addr, uint8_t val);

void nand_get_feature(struct sfc_transfer *transfer, struct cmd_info *cmd, uint8_t addr, uint8_t *val);

#endif
