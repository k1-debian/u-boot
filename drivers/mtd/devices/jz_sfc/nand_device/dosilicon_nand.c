#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include <asm/arch/spinand.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"

#define THOLD	    5
#define TSETUP	    5
#define TSHSL_R	    100
#define TSHSL_W	    100

#define TRD	    90
#define TPP	    700
#define TBE	    10

static struct jz_sfcnand_base_param dosilicon_param[] = {
	[0] = {
		/*DS35Q1GAXXX*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tHOLD  = THOLD,
		.tSETUP = TSETUP,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 70,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x4,
		.need_quad = 1,
	},
	[1] = {
		/*DS35Q2GAXXX*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tHOLD  = THOLD,
		.tSETUP = TSETUP,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 90,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x4,
		.need_quad = 1,
	},
	[2] = {
		/*DS35Q2GBXXX*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tHOLD  = THOLD,
		.tSETUP = TSETUP,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 120,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
		.need_quad = 1,
	},
	[3] = {
		/*DS35M1GAXXX-1.8V*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tHOLD  = THOLD,
		.tSETUP = TSETUP,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 80,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x4,
		.need_quad = 1,
	},
	[4] = {
		/*DS35Q2GAXXX-1V8*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tHOLD  = THOLD,
		.tSETUP = TSETUP,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 90,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x4,
		.need_quad = 1,
	},
	[5] = {
		/*DS35X1GBXXX*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tHOLD  = THOLD,
		.tSETUP = TSETUP,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 120,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
		.need_quad = 1,
	},
	[6] = {
		/*DS35Q4GBXXX*/
		/*Power outage test file system damage, resulting in inability to start normally
		 *During the power on and power off test, the file system was changed to read-only,
		 * and the data partition was used for reading and writing. However, during the test,
		 * the data partition was unable to write files properly*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 4096,

		.tHOLD  = THOLD,
		.tSETUP = TSETUP,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 120,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
		.need_quad = 1,
	},
        [7] = {
		/*DS35Q12C-1B*/
		/*this parameter has not been tested for high/low temperature and random power on/off.*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 512,

		.tHOLD  = THOLD,
		.tSETUP = TSETUP,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 120,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
		.need_quad = 1,
	},
	[8] = {
		/*LC35Q4GMXXX*/
		/* Not aging test */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 4096,

		.tHOLD  = THOLD,
		.tSETUP = TSETUP,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 120,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
		.need_quad = 1,
	},
};

static struct device_id_struct device_id[] = {
	DEVICE_ID_STRUCT(0x71, "DS35Q1GAXXX",     &dosilicon_param[0]),
	DEVICE_ID_STRUCT(0x72, "DS35Q2GAXXX",     &dosilicon_param[1]),
	DEVICE_ID_STRUCT(0xF2, "DS35Q2GBXXX",     &dosilicon_param[2]),
	DEVICE_ID_STRUCT(0x21, "DS35M1GAXXX-1V8", &dosilicon_param[3]),
	DEVICE_ID_STRUCT(0x22, "DS35Q2GAXXX-1V8", &dosilicon_param[4]),
	DEVICE_ID_STRUCT(0xF1, "DS35X1GBXXX",     &dosilicon_param[5]),
	DEVICE_ID_STRUCT(0xB4, "DS35Q4GBXXX",     &dosilicon_param[6]),
	DEVICE_ID_STRUCT(0x75, "DS35Q12C-1B",     &dosilicon_param[7]),
	DEVICE_ID_STRUCT(0xF4, "LC35X4GMXXX",     &dosilicon_param[8]),
};

static void dosilicon_pageread_to_cache(struct sfc_transfer *transfer, struct flash_operation_message *op_info) {

	transfer->cmd_info.cmd = SPINAND_CMD_PARD;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = op_info->pageaddr;
	transfer->addr_len = 3;

	transfer->cmd_info.dataen = DISABLE;
	transfer->len = 0;

	transfer->data_dummy_bits = 0;
	transfer->ops_mode = CPU_OPS;
	return;
}

static int32_t dosilicon_get_read_feature(struct flash_operation_message *op_info) {

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
		case 0x71:
		case 0x72:
		case 0x21:
		case 0x22:
			switch((ecc_status >> 4) & 0x3) {
				case 0x0:
					return 0;
				case 0x1:
					return 4;
				case 0x2:
					return -EBADMSG;
				default:
					break;
			}
			break;
		case 0xF2:
		case 0xF1:
			switch((ecc_status >> 4) & 0x7) {
				case 0x0:
				case 0x1:
					return 0;
				case 0x2:
					return -EBADMSG;
				case 0x3:
				case 0x5:
					return 8;
				default:
					break;
			}
			break;
		case 0xB4:
		case 0x75:
		case 0xF4:
			switch((ecc_status >> 4) & 0x7) {
				case 0x0:
					return 0;
				case 0x1:
					return 3;
				case 0x2:
					return -EBADMSG;
					break;
				case 0x3:
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

static void dosilicon_single_read(struct sfc_transfer *transfer, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_sfcnand_flashinfo *nand_info = flash->flash_info;
	uint16_t device_id = nand_info->id_device;
	uint32_t columnaddr = op_info->columnaddr;
	int plane_flag = 0;

	switch(device_id) {
		case 0x71:
		case 0x21:
			break;
		case 0x72:
		case 0xF2:
		case 0x22:
		case 0xF1:
		case 0xB4:
		case 0x75:
		case 0xF4:
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

static void dosilicon_quad_read(struct sfc_transfer *transfer, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_sfcnand_flashinfo *nand_info = flash->flash_info;
	uint16_t device_id = nand_info->id_device;
	uint32_t columnaddr = op_info->columnaddr;
	int plane_flag = 0;

	switch(device_id) {
	    case 0x71:
	    case 0x21:
			break;
	    case 0x72:
	    case 0xF2:
		case 0xF1:
		case 0x22:
		case 0xB4:
		case 0x75:
		case 0xF4:
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

static void dosilicon_single_load(struct sfc_transfer *transfer, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_sfcnand_flashinfo *nand_info = flash->flash_info;
	uint16_t device_id = nand_info->id_device;
	uint32_t columnaddr = op_info->columnaddr;
	int plane_flag = 0;

	switch(device_id) {
		case 0x71:
		case 0x21:
			break;
		case 0x72:
		case 0xF2:
		case 0x22:
		case 0xF1:
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

static void dosilicon_quad_load(struct sfc_transfer *transfer, struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct jz_sfcnand_flashinfo *nand_info = flash->flash_info;
	uint16_t device_id = nand_info->id_device;
	uint32_t columnaddr = op_info->columnaddr;
	int plane_flag = 0;

	switch(device_id) {
		case 0x71:
		case 0x21:
			break;
		case 0x72:
		case 0xF2:
		case 0x22:
		case 0xF1:
		case 0xB4:
		case 0x75:
		case 0xF4:
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

static void dosilicon_program_exec(struct sfc_transfer *transfer, struct flash_operation_message *op_info) {

	transfer->cmd_info.cmd = SPINAND_CMD_PRO_EN;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = op_info->pageaddr;
	transfer->addr_len = 3;

	transfer->cmd_info.dataen = DISABLE;
	transfer->len = 0;

	transfer->data_dummy_bits = 0;
	transfer->ops_mode = CPU_OPS;
}

static int dosilicon_nand_init(void) {
	struct jz_sfcnand_device *dosilicon_nand;
	dosilicon_nand = kzalloc(sizeof(*dosilicon_nand), GFP_KERNEL);
	if(!dosilicon_nand) {
		pr_err("alloc dosilicon_nand struct fail\n");
		return -ENOMEM;
	}

	dosilicon_nand->id_manufactory = 0xE5;
	dosilicon_nand->id_device_list = device_id;
	dosilicon_nand->id_device_count = ARRAY_SIZE(dosilicon_param);

	dosilicon_nand->ops.nand_read_ops.pageread_to_cache = dosilicon_pageread_to_cache;
	dosilicon_nand->ops.nand_read_ops.get_feature = dosilicon_get_read_feature;
	dosilicon_nand->ops.nand_read_ops.single_read = dosilicon_single_read;
	dosilicon_nand->ops.nand_read_ops.quad_read = dosilicon_quad_read;

	dosilicon_nand->ops.nand_write_ops.single_load = dosilicon_single_load;
	dosilicon_nand->ops.nand_write_ops.quad_load = dosilicon_quad_load;
	dosilicon_nand->ops.nand_write_ops.program_exec = dosilicon_program_exec;

	return jz_sfcnand_register(dosilicon_nand);
}
SPINAND_MOUDLE_INIT(dosilicon_nand_init);
