#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include "../jz_sfc_nand.h"
#include "nand_common.h"

#define MXIC_DEVICES_NUM         3
#define MXIC_CMD_GET_ECC	0x7c
#define THOLD	    4
#define TSETUP	    4
#define TSHSL_R	    100
#define TSHSL_W	    100

static struct jz_nand_base_param mxic_param[MXIC_DEVICES_NUM] = {
	[0] = {
	/*MX35LF1GE4AB*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tHOLD  = THOLD,
		.tSETUP = TSETUP,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.ecc_max = 0x4,
		.need_quad = 1,
	},
	[1] = {
	/*MX35LF2GE4AB*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tHOLD  = THOLD,
		.tSETUP = TSETUP,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.ecc_max = 0x4,
		.need_quad = 1,
	},
	[2] = {
	/*MX35LF2G14AC*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tHOLD  = THOLD,
		.tSETUP = TSETUP,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.ecc_max = 0,
		.need_quad = 1,
	},

};

static struct device_id_struct device_id[MXIC_DEVICES_NUM] = {
	DEVICE_ID_STRUCT(0x12, "MX35LF1GE4AB", &mxic_param[0]),
	DEVICE_ID_STRUCT(0x22, "MX35LF2GE4AB", &mxic_param[1]),
	DEVICE_ID_STRUCT(0x20, "MX35LF2G14AC", &mxic_param[2]),
};

static uint8_t plane_select = 0;

static void mxic_pageread_to_cache(struct sfc_flash *flash, struct sfc_transfer *transfer, struct cmd_info *cmd, uint32_t pageaddr, uint8_t device_id) {

	switch(device_id) {
	    case 0x20:
	    case 0x22:
		    {
			    uint32_t blockaddr;
			    if(pageaddr > (64 * 1024 - 1)) {
				    pageaddr -= 65536;
				    blockaddr =	(pageaddr & (~(64 - 1))) << 1;
				    pageaddr = blockaddr | (1 << 6) | (pageaddr & (64 - 1));
				    plane_select = 1;
			    } else {
				    blockaddr =	(pageaddr & (~(64 - 1))) << 1;
				    pageaddr = blockaddr | (pageaddr & (64 - 1));
				    plane_select = 0;
			    }
		    }
		    break;
	    case 0x12:
		    break;
	    default:
		    pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
		    break;
	}

	cmd->cmd = SPINAND_CMD_PARD;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = pageaddr;
	transfer->addr_len = 3;

	cmd->dataen = DISABLE;
	transfer->len = 0;

	transfer->data_dummy_bits = 0;
	transfer->cmd_info = cmd;
	transfer->ops_mode = CPU_OPS;
	return;
}

static int32_t get_ecc_value(struct sfc_flash *flash) {
	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;
	uint32_t buf = 0;
	int8_t count = 5;

	sfc_message_init(&message);
try_read_again:
	memset(&transfer, 0, sizeof(transfer));
	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd = MXIC_CMD_GET_ECC;
	transfer.sfc_mode = TM_STD_SPI;

	transfer.addr = 0;
	transfer.addr_len = 0;

	cmd.dataen = ENABLE;
	transfer.len = 1;
	transfer.data = (void *)&buf;
	transfer.direction = GLB_TRAN_DIR_READ;

	transfer.data_dummy_bits = 8;
	transfer.cmd_info = &cmd;
	transfer.ops_mode = CPU_OPS;

	sfc_message_add_tail(&transfer, &message);
	if(sfc_sync(flash->sfc, &message) && count--)
		goto try_read_again;
	if(count < 0)
		return -EIO;
	return buf;
}

static int32_t mxic_get_read_feature(struct sfc_flash *flash, uint8_t device_id) {
	struct sfc_transfer transfer;
	struct sfc_message message;
	struct cmd_info cmd;
	uint8_t ecc_status = 0;
	int32_t ret = 0;

	memset(&transfer, 0, sizeof(transfer));
	memset(&cmd, 0, sizeof(cmd));
	sfc_message_init(&message);

	cmd.cmd = SPINAND_CMD_GET_FEATURE;
	transfer.sfc_mode = TM_STD_SPI;

	transfer.addr = SPINAND_ADDR_STATUS;
	transfer.addr_len = 1;

	cmd.dataen = DISABLE;
	transfer.len = 0;

	transfer.data_dummy_bits = 0;
	cmd.sta_exp = (0 << 0);
	cmd.sta_msk = SPINAND_IS_BUSY;
	transfer.cmd_info = &cmd;
	transfer.ops_mode = CPU_OPS;

	sfc_message_add_tail(&transfer, &message);
	if(sfc_sync(flash->sfc, &message)) {
	        printf("sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	ecc_status = sfc_get_sta_rt(flash->sfc);
	switch(device_id) {
		case 0x12:
		case 0x22:
			switch((ecc_status >> 0x4) & 0x3) {
			    case 0x0:
				    ret = 0;
				    break;
			    case 0x1:
				    ret = get_ecc_value(flash);
				    break;
			    case 0x2:
				    ret = -EBADMSG;
				    break;
			    default:
				   printf("it is flash Unknown state, device_id: 0x%02x\n", device_id);
				    ret = -EIO;
			}
			break;
		case 0x20:
			ret = 0;
			break;
		default:
			printf("device_id err, it maybe don`t support this device, check your device id: device_id = 0x%02x\n", device_id);
		ret = -EIO;
	}
	return ret;
}

static void mxic_single_read(struct sfc_transfer *transfer, struct cmd_info *cmd, uint32_t columnaddr, void *buffer, uint32_t len, uint32_t device_id) {

	switch(device_id) {
	    case 0x20:
	    case 0x22:
		    if(plane_select)
			    columnaddr |= (1 << 12);
		    else
			    columnaddr &= ~(1 << 12);
		    plane_select = 0;
		    break;
	    case 0x12:
		    break;
	    default:
		    pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
		    break;
	}

	cmd->cmd = SPINAND_CMD_FRCH;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = columnaddr;
	transfer->addr_len = 2;

	cmd->dataen = ENABLE;
	transfer->data = buffer;
	transfer->len = len;
	transfer->direction = GLB_TRAN_DIR_READ;

	transfer->data_dummy_bits = 8;
	transfer->cmd_info = cmd;
	transfer->ops_mode = DMA_OPS;
	return;
}

static void mxic_quad_read(struct sfc_transfer *transfer, struct cmd_info *cmd, uint32_t columnaddr, void *buffer, uint32_t len, uint32_t device_id) {

	switch(device_id) {
	    case 0x20:
	    case 0x22:
		    if(plane_select)
			    columnaddr |= (1 << 12);
		    else
			    columnaddr &= ~(1 << 12);
		    plane_select = 0;
		    break;
	    case 0x12:
		    break;
	    default:
		    pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
		    break;
	}

	cmd->cmd = SPINAND_CMD_RDCH_X4;
	transfer->sfc_mode = TM_QI_QO_SPI;

	transfer->addr = columnaddr;
	transfer->addr_len = 2;

	cmd->dataen = ENABLE;
	transfer->data = buffer;
	transfer->len = len;
	transfer->direction = GLB_TRAN_DIR_READ;

	transfer->data_dummy_bits = 8;
	transfer->cmd_info = cmd;
	transfer->ops_mode = DMA_OPS;
	return;
}

static void mxic_single_load(struct sfc_transfer *transfer, struct cmd_info *cmd, uint32_t columnaddr, void *buffer, uint32_t len, uint8_t device_id, uint32_t pageaddr) {

	switch(device_id) {
		case 0x20:
		case 0x22:
			if(pageaddr > 65535) {
				columnaddr |= (1 << 12);
			} else {
				columnaddr &= ~(1 << 12);
			}
			break;
		case 0x12:
		    break;
	    default:
		    pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
		    break;
	}

	cmd->cmd = SPINAND_CMD_PRO_LOAD;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = columnaddr;
	transfer->addr_len = 2;

	cmd->dataen = ENABLE;
	transfer->data = buffer;
	transfer->len = len;
	transfer->direction = GLB_TRAN_DIR_WRITE;

	transfer->data_dummy_bits = 0;
	transfer->cmd_info = cmd;
	transfer->ops_mode = DMA_OPS;
}

static void mxic_quad_load(struct sfc_transfer *transfer, struct cmd_info *cmd, uint32_t columnaddr, void *buffer, uint32_t len, uint8_t device_id, uint32_t pageaddr) {

	switch(device_id) {
		case 0x20:
		case 0x22:
			if(pageaddr > 65535) {
				columnaddr |= (1 << 12);
			} else {
				columnaddr &= ~(1 << 12);
			}
			break;
		case 0x12:
		    break;
	    default:
		    pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
		    break;
	}

	cmd->cmd = SPINAND_CMD_PRO_LOAD_X4;
	transfer->sfc_mode = TM_QI_QO_SPI;

	transfer->addr = columnaddr;
	transfer->addr_len = 2;

	cmd->dataen = ENABLE;
	transfer->data = buffer;
	transfer->len = len;
	transfer->direction = GLB_TRAN_DIR_WRITE;

	transfer->data_dummy_bits = 0;
	transfer->cmd_info = cmd;
	transfer->ops_mode = DMA_OPS;

}

static void mxic_program_exec(struct sfc_transfer *transfer, struct cmd_info *cmd, uint32_t pageaddr, uint8_t device_id) {

	switch(device_id) {
	    case 0x20:
	    case 0x22:
		    {
			    uint32_t blockaddr;
			    if(pageaddr > (64 * 1024 - 1)) {
				    pageaddr -= 65536;
				    blockaddr =	(pageaddr & (~(64 - 1))) << 1;
				    pageaddr = blockaddr | (1 << 6) | (pageaddr & (64 - 1));
			    } else {
				    blockaddr =	(pageaddr & (~(64 - 1))) << 1;
				    pageaddr = blockaddr | (pageaddr & (64 - 1));
			    }
		    }
		    break;
	    case 0x12:
		    break;
	    default:
		    pr_err("device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
		    break;
	}

	cmd->cmd = SPINAND_CMD_PRO_EN;
	transfer->sfc_mode = TM_STD_SPI;

	transfer->addr = pageaddr;
	transfer->addr_len = 3;

	cmd->dataen = DISABLE;
	transfer->len = 0;

	transfer->data_dummy_bits = 0;
	transfer->cmd_info = cmd;
	transfer->ops_mode = CPU_OPS;
}

int mxic_nand_init(void) {
	struct jz_nand_device *mxic_nand;
	mxic_nand = kzalloc(sizeof(*mxic_nand), GFP_KERNEL);
	if(!mxic_nand) {
		pr_err("alloc mxic_nand struct fail\n");
		return -ENOMEM;
	}

	mxic_nand->id_manufactory = 0xC2;
	mxic_nand->id_device_list = device_id;
	mxic_nand->id_device_count = MXIC_DEVICES_NUM;

	mxic_nand->ops.nand_read_ops.pageread_to_cache = mxic_pageread_to_cache;
	mxic_nand->ops.nand_read_ops.get_feature = mxic_get_read_feature;
	mxic_nand->ops.nand_read_ops.single_read = mxic_single_read;
	mxic_nand->ops.nand_read_ops.quad_read = mxic_quad_read;

        mxic_nand->ops.nand_write_ops.single_load = mxic_single_load;
        mxic_nand->ops.nand_write_ops.quad_load = mxic_quad_load;
	mxic_nand->ops.nand_write_ops.program_exec = mxic_program_exec;

	return jz_spinand_register(mxic_nand);
}
SPINAND_MOUDLE_INIT(mxic_nand_init);
