#include <cloner/cloner.h>
#include "cloner_moudle.h"
#include <efuse.h>

static int enabled = 0;

static int32_t clmd_efuse_read(struct cloner *cloner, int sub_type, void *ops_data)
{
	off_t offset;
	int ret = 0;
	void *addr = (void *)cloner->read_req->buf;
	u32 length = cloner->read_req->length;

	if(!enabled) {
		ret = efuse_init(efuse_args->efuse_en_gpio, efuse_args->efuse_en_active);
		if(ret < 0) {
			LOG_ERROR("efuse init error\n");
			return ret;
		}
		enabled = 1;
	}

	offset = cloner->cmd->read.partition + cloner->cmd->read.offset;
	ret = efuse_read_segment(addr, length, offset);
	if (ret < 0)
		LOG_ERROR("efuse read error\n");

	return ret;
}

static int32_t clmd_efuse_write(struct cloner *cloner, int sub_type, void *ops_data)
{
	off_t offset;
	u32 length;
	void *addr;
	int ret = 0;

	if(!enabled) {
		ret = efuse_init(efuse_args->efuse_en_gpio, efuse_args->efuse_en_active);
		if(ret < 0) {
			LOG_ERROR("efuse init error\n");
			return ret;
		}
		enabled = 1;
	}

	offset = cloner->cmd->write.partition + cloner->cmd->write.offset;
	length = cloner->write_req->actual ? cloner->write_req->actual : cloner->cmd->write.length;
	addr = (void *)cloner->write_req->buf;

	ret = efuse_write_segment(addr, length, offset);
	if (ret)
		LOG_ERROR("efuse write error\n");

	return ret;
}

int cloner_efuse_init(void)
{
	struct cloner_moudle *clmd = malloc(sizeof(struct cloner_moudle));
	int ret;

	if (!clmd)
		return -ENOMEM;
	clmd->medium = MAGIC_EFUSE;
	clmd->ops = EFUSE;
	clmd->write = clmd_efuse_write;
	clmd->init = NULL;
	clmd->info = NULL;
	clmd->read = clmd_efuse_read;
	clmd->check = NULL;
	clmd->data = NULL;
//	printf("cloner efuse register\n");
	return register_cloner_moudle(clmd);
}
CLONER_MOUDLE_INIT(cloner_efuse_init);
