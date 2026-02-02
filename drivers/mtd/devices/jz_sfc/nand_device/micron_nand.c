#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include <asm/arch/spinand.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"

#define TSETUP		5
#define THOLD		5
#define	TSHSL_R		50
#define	TSHSL_W		50

#define TRD		80
#define TPP		600
#define TBE		10


static struct jz_sfcnand_base_param micron_param[] = {

	[0] = {
		/* MT29F1G01ABBFDSF-1.8V */
		.pagesize = 2048,
		.blocksize = 2048 * 64,
		.oobsize = 128,
		.flashsize = 2048 * 64 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
		.need_quad = 1,
	},

};

static struct device_id_struct device_id[] = {
	DEVICE_ID_STRUCT(0x15, "MT29F1G01ABBFDSF-1.8V", &micron_param[0]),
};

static int32_t micron_get_read_feature(struct flash_operation_message *op_info)
{

	struct sfc_flash *flash = op_info->flash;
	struct jz_sfcnand_flashinfo *nand_info = flash->flash_info;
	struct sfc_transfer transfer;
	uint16_t device_id = nand_info->id_device;
	uint8_t ecc_status = 0;
	int32_t ret = 0;

retry:
	ecc_status = 0;
	memset(&transfer, 0, sizeof(transfer));
	sfc_list_init(&transfer);

	transfer.cmd_info.cmd = SPINAND_CMD_GET_FEATURE;
	transfer.sfc_mode = TM_STD_SPI;

	transfer.addr = SPINAND_ADDR_STATUS;
	transfer.addr_len = 1;

	transfer.cmd_info.dataen = ENABLE;
	transfer.data = &ecc_status;
	transfer.len = 1;
	transfer.direction = GLB_TRAN_DIR_READ;

	transfer.data_dummy_bits = 0;
	transfer.ops_mode = CPU_OPS;

	if(sfc_sync(flash->sfc, &transfer)) {
	        printf("sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	if(ecc_status & SPINAND_IS_BUSY)
		goto retry;

	switch(device_id) {
		case 0x15:
			switch((ecc_status >> 4) & 0x7) {
				case 0x0:
				case 0x1:
					return 3;
				case 0x2:
					return -EBADMSG;
				case 0x3:
					return 6;
				case 0x5:
					return 8;
				default:
					break;
			}
			break;
		default:
			printf("device_id err, it maybe don`t support this device, check your device id: device_id = 0x%02x\n", device_id);
			break;
	}
	return -EINVAL;
}

static void micron_single_read(struct sfc_transfer *transfer, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_sfcnand_flashinfo *nand_info = flash->flash_info;
	uint16_t device_id = nand_info->id_device;
	uint32_t columnaddr = op_info->columnaddr;
	int plane_flag = 0;

	switch(device_id) {
		case 0x34:
			plane_flag = (op_info->pageaddr >> 6) & 1;
			columnaddr |= (plane_flag << 12);
			break;
	    default:
		    pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
		    break;
	}

	transfer->cmd_info.cmd = SPINAND_CMD_FRCH;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = columnaddr;
	transfer->addr_len = 2;

	transfer->cmd_info.dataen = ENABLE;
	transfer->data = op_info->buffer;
	transfer->len = op_info->len;
	transfer->direction = GLB_TRAN_DIR_READ;

	transfer->data_dummy_bits = 8;
	transfer->ops_mode = DMA_OPS;
	return;
}

static void micron_quad_read(struct sfc_transfer *transfer, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_sfcnand_flashinfo *nand_info = flash->flash_info;
	uint16_t device_id = nand_info->id_device;
	uint32_t columnaddr = op_info->columnaddr;
	int plane_flag = 0;

	switch(device_id) {
		case 0x34:
			plane_flag = (op_info->pageaddr >> 6) & 1;
			columnaddr |= (plane_flag << 12);
			break;
	    default:
		    pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
		    break;
	}

	transfer->cmd_info.cmd = SPINAND_CMD_RDCH_X4;
	transfer->sfc_mode = TM_QI_QO_SPI;

	transfer->addr = columnaddr;
	transfer->addr_len = 2;

	transfer->cmd_info.dataen = ENABLE;
	transfer->data = op_info->buffer;
	transfer->len = op_info->len;
	transfer->direction = GLB_TRAN_DIR_READ;

	transfer->data_dummy_bits = 8;
	transfer->ops_mode = DMA_OPS;
	return;
}

static void micron_single_load(struct sfc_transfer *transfer, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_sfcnand_flashinfo *nand_info = flash->flash_info;
	uint16_t device_id = nand_info->id_device;
	uint32_t columnaddr = op_info->columnaddr;
	int plane_flag = 0;

	switch(device_id) {
		case 0x34:
			plane_flag = (op_info->pageaddr >> 6) & 1;
			columnaddr |= (plane_flag << 12);
			break;
		default:
		    pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
		    break;
	}

	transfer->cmd_info.cmd = SPINAND_CMD_PRO_LOAD;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = columnaddr;
	transfer->addr_len = 2;

	transfer->cmd_info.dataen = ENABLE;
	transfer->data = op_info->buffer;
	transfer->len = op_info->len;
	transfer->direction = GLB_TRAN_DIR_WRITE;

	transfer->data_dummy_bits = 0;
	transfer->ops_mode = DMA_OPS;
}

static void micron_quad_load(struct sfc_transfer *transfer, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_sfcnand_flashinfo *nand_info = flash->flash_info;
	uint16_t device_id = nand_info->id_device;
	uint32_t columnaddr = op_info->columnaddr;
	int plane_flag = 0;

	switch(device_id) {
		case 0x34:
			plane_flag = (op_info->pageaddr >> 6) & 1;
			columnaddr |= (plane_flag << 12);
			break;
	    default:
		    pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
		    break;
	}

	transfer->cmd_info.cmd = SPINAND_CMD_PRO_LOAD_X4;
	transfer->sfc_mode = TM_QI_QO_SPI;

	transfer->addr = columnaddr;
	transfer->addr_len = 2;

	transfer->cmd_info.dataen = ENABLE;
	transfer->data = op_info->buffer;
	transfer->len = op_info->len;
	transfer->direction = GLB_TRAN_DIR_WRITE;

	transfer->data_dummy_bits = 0;
	transfer->ops_mode = DMA_OPS;

}

static int micron_nand_init(void) {
	struct jz_sfcnand_device *micron_nand;
	micron_nand = kzalloc(sizeof(*micron_nand), GFP_KERNEL);
	if(!micron_nand) {
		pr_err("alloc micron_nand struct fail\n");
		return -ENOMEM;
	}

	micron_nand->id_manufactory = 0x2C;
	micron_nand->id_device_list = device_id;
	micron_nand->id_device_count = ARRAY_SIZE(micron_param);

	micron_nand->ops.nand_read_ops.get_feature = micron_get_read_feature;
	micron_nand->ops.nand_read_ops.single_read = micron_single_read;
	micron_nand->ops.nand_read_ops.quad_read = micron_quad_read;
	micron_nand->ops.nand_write_ops.single_load = micron_single_load;
	micron_nand->ops.nand_write_ops.quad_load = micron_quad_load;

	return jz_sfcnand_register(micron_nand);
}

SPINAND_MOUDLE_INIT(micron_nand_init);
