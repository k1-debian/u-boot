#ifndef __SPINOR_H
#define __SPINOR_H
#include <asm/arch/spinor_cmd.h>

#define SIZEOF_NAME			32

#define NOR_MAGIC	0x726f6e	//ascii "nor"
#define NOR_MAJOR_VERSION_NUMBER        2
#define NOR_MINOR_VERSION_NUMBER        0
#define NOR_REVERSION_NUMBER    0
#define NOR_VERSION             (NOR_MAJOR_VERSION_NUMBER | (NOR_MINOR_VERSION_NUMBER << 8) | (NOR_REVERSION_NUMBER << 16))

#define NOR_PART_NUM	10


struct spi_nor_cmd_info {
	unsigned short cmd;
	unsigned char dummy_byte;
	unsigned char addr_nbyte;
	unsigned char transfer_mode;

};

struct spi_nor_st_info {
	unsigned short cmd;
	unsigned char bit_shift;
	unsigned char mask;
	unsigned char val;
	unsigned char len; //length of byte to operate from register
	unsigned char dummy;
};

struct spi_nor_info {
	unsigned char name[32];
	unsigned int id;

	struct spi_nor_cmd_info read_standard;
	struct spi_nor_cmd_info read_quad;

	struct spi_nor_cmd_info write_standard;
	struct spi_nor_cmd_info write_quad;

	struct spi_nor_cmd_info sector_erase;

	struct spi_nor_cmd_info wr_en;
	struct spi_nor_cmd_info en4byte;
	struct spi_nor_st_info	quad_set;
	struct spi_nor_st_info	quad_get;
	struct spi_nor_st_info	busy;

	unsigned short quad_ops_mode;
	unsigned short addr_ops_mode;

	unsigned int tCHSH;      //hold
	unsigned int tSLCH;      //setup
	unsigned int tSHSL_RD;   //interval
	unsigned int tSHSL_WR;

	unsigned int chip_size;
	unsigned int page_size;
	unsigned int erase_size;

	unsigned char chip_erase_cmd;
};

struct mini_spi_nor_info {
	unsigned char name[32];
	unsigned int id;

	struct spi_nor_cmd_info read_standard;
	struct spi_nor_cmd_info read_quad;
	struct spi_nor_cmd_info wr_en;
	struct spi_nor_cmd_info en4byte;

	struct spi_nor_st_info	quad_set;
	struct spi_nor_st_info	quad_get;
	struct spi_nor_st_info	busy;

	unsigned short quad_ops_mode;
	unsigned short addr_ops_mode;

	unsigned int chip_size;
	unsigned int page_size;
	unsigned int erase_size;

};


#define MTD_MODE                0x0     //use mtd mode, erase partition when write
#define MTD_D_MODE              0x2     //use mtd dynamic mode, erase block_size when write
#define UBI_MANAGER             0x1

struct nor_partition {
	char name[32];
	uint32_t size;
	uint32_t offset;
	uint32_t mask_flags;//bit 0-1 mask the partiton RW mode, 0:RW  1:W  2:R
	uint32_t manager_mode;
};

struct norflash_partitions {
	struct nor_partition nor_partition[NOR_PART_NUM];
	uint32_t num_partition_info;
};

struct nor_private_data {
	unsigned int fs_erase_size;
	unsigned char uk_quad;
};

struct burner_params {
	uint32_t magic;
	uint32_t version;
	struct spi_nor_info spi_nor_info;
	struct norflash_partitions norflash_partitions;
	struct nor_private_data nor_pri_data;
};

struct spiflash_info {
	struct burner_params burner_params;
	struct mini_spi_nor_info mini_spi_nor_info;
};

struct spi_nor_flash_ops {
	int (*set_4byte_mode)(struct sfc_flash *flash);
	int (*set_quad_mode)(struct sfc_flash *flash);
};

struct spinor_flashinfo {

	int quad_succeed;
	struct spi_nor_flash_ops *nor_flash_ops;
	struct spi_nor_info *nor_flash_info;
	struct spi_nor_cmd_info *cur_r_cmd;
	struct spi_nor_cmd_info *cur_w_cmd;
	struct norflash_partitions *norflash_partitions;
	struct nor_private_data *nor_pri_data;

};

struct nor_block_info {
	unsigned int blocksize;
	unsigned char cmd_blockerase;
	/* MAX Busytime for block erase, unit: ms */
	unsigned int be_maxbusy;
};

struct quad_mode {
	unsigned char dummy_byte;
	unsigned char RDSR_CMD;
	unsigned char WRSR_CMD;
	unsigned int RDSR_DATA;//the data is write the spi status register for QE bit
	unsigned int RD_DATA_SIZE;//the data is write the spi status register for QE bit
	unsigned int WRSR_DATA;//this bit should be the flash QUAD mode enable
	unsigned int WD_DATA_SIZE;//the data is write the spi status register for QE bit
	unsigned char cmd_read;
	unsigned char sfc_mode;
};
struct nor_params {
	char name[SIZEOF_NAME];
	unsigned int pagesize;
	unsigned int sectorsize;
	unsigned int chipsize;
	unsigned int erasesize;
	int id;
	/* Flash Address size, unit: Bytes */
	int addrsize;

	/* MAX Busytime for page program, unit: ms */
	unsigned int pp_maxbusy;
	/* MAX Busytime for sector erase, unit: ms */
	unsigned int se_maxbusy;
	/* MAX Busytime for chip erase, unit: ms */
	unsigned int ce_maxbusy;

	/* Flash status register num, Max support 3 register */
	int st_regnum;
	/* Some NOR flash has different blocksize and block erase command,
	 *          * One command with One blocksize. */
	struct nor_block_info block_info;
	struct quad_mode quad_mode;
};
struct legacy_params {
	uint32_t magic;
	uint32_t version;
	struct nor_params nor_params;
	struct norflash_partitions norflash_partitions;

};


#endif
