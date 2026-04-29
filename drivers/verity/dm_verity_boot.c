#include <common.h>
#include <bootargs_mem.h>
#include <command.h>
#include <config.h>
#include <asm/errno.h>
#include <dm_verity_boot.h>
#include <malloc.h>
#include <mmc.h>
#include <part.h>
#include <security_pubkey.h>
#include <sha256.h>

#include "dm_verity_manifest_source.h"
#include "dm_verity_media.h"
#include "dm_verity_pubkey_source.h"

#if defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_SECURE_SUPPORT)
#include <asm/arch/cpm.h>
#include <asm/io.h>
#include "../scboot/jz_sec_v4/secall.h"
#endif

#ifndef CONFIG_DM_VERITY_HASH_BY_SC_ENABLE
#define CONFIG_DM_VERITY_HASH_BY_SC_ENABLE	0
#endif

#define DM_VERITY_TREE_READ_CHUNK	65536U

struct dm_verity_data_runtime {
	struct dm_verity_data_source_desc source;
};

#if defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_SECURE_SUPPORT) && \
	CONFIG_DM_VERITY_HASH_BY_SC_ENABLE
extern void flush_cache_all(void);
extern void pdma_wait(void);
#endif

static int dm_verity_data_read_region(void *priv, const char *part_name,
				      unsigned int offset,
				      void *buf, unsigned int size)
{
	const struct dm_verity_data_runtime *runtime = priv;

	if (!runtime || !part_name || !buf || !size)
		return -EINVAL;

	return dm_verity_read_data_source_region(&runtime->source, part_name,
						 offset, buf, size);
}

#if defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_SECURE_SUPPORT) && \
	CONFIG_DM_VERITY_HASH_BY_SC_ENABLE
static void dm_verity_sc_prepare_runtime_hw(void)
{
	unsigned int *pdma_ins = (unsigned int *)pdma_wait;
	volatile unsigned int *pdma_bank0 =
		(volatile unsigned int *)TCSM_BANK0;
	unsigned int i;

	REG32(CPM_BASE + CPM_CLKGR0) = 0;
	REG32(CPM_BASE + CPM_CLKGR1) = 0;

	reset_mcu();
	for (i = 0; i < 6; i++)
		pdma_bank0[i] = pdma_ins[i];

	boot_up_mcu();
	mdelay(500);
}

static int dm_verity_sc_runtime_init(void)
{
	static int initialized;

	if (initialized)
		return 0;

	dm_verity_sc_prepare_runtime_hw();
	initialized = 1;
	return 0;
}

static int dm_verity_sc_sha256_begin(void)
{
	return dm_verity_sc_runtime_init();
}

static int dm_verity_sc_sha256_update(const unsigned char *data,
			      unsigned int size,
			      int newround,
			      int endround,
			      unsigned char digest[SHA256_SUM_LEN])
{
	volatile struct sc_args *args = GET_SC_ARGS();
	volatile unsigned char *tcsm_in =
		(volatile unsigned char *)MCU_TCSM_INDATA;
	volatile unsigned char *tcsm_out =
		(volatile unsigned char *)MCU_TCSM_NKUSIG;
	volatile unsigned int *retval =
		(volatile unsigned int *)MCU_TCSM_RETVAL;
	if (!data || !size || (size & 0x3) || size > SC_MAX_SIZE_PERTIME)
		return -EINVAL;

	memcpy((void *)tcsm_in, data, size);
	args->arg[0] = (size / 4) |
		(newround ? HASH_NEWROUND : 0) |
		(endround ? HASH_ENDROUND : 0) |
		HASH_SET(HASH_SELECT_SHA256);
	args->arg[1] = MCU_TCSM_PADDR(tcsm_in);
	args->arg[2] = MCU_TCSM_PADDR(tcsm_out);

	*retval = 0;
	flush_cache_all();
	(void)secall(args, SC_FUNC_HASH, 0, 1);
	flush_cache_all();
	if (*retval != SC_ERR_SUCC)
		return -EIO;

	if (endround)
		memcpy(digest, (const void *)tcsm_out, SHA256_SUM_LEN);

	return 0;
}

static int dm_verity_hash_region(void *priv, const char *part_name,
			 unsigned int offset,
			 unsigned int size,
			 unsigned char *digest,
			 unsigned int digest_len)
{
	const struct dm_verity_data_runtime *runtime = priv;
	struct mmc *mmc = NULL;
	disk_partition_t part_info;
	unsigned char *chunk;
	unsigned int done = 0;
	unsigned int fed;
	unsigned int this_size;
	unsigned int hash_size;
	int ret;
	int newround = 1;

	if (!runtime || !part_name || !digest || digest_len != SHA256_SUM_LEN ||
	    !size)
		return -EINVAL;
	if (runtime->source.media != DM_VERITY_SOURCE_MEDIA_MMC)
		return -ENOSYS;

	ret = dm_verity_get_mmc_partition_info_by_name(
		runtime->source.location.mmc.dev,
		runtime->source.location.mmc.max_part,
		part_name, &mmc, &part_info);
	if (ret)
		return ret;

	chunk = malloc(DM_VERITY_TREE_READ_CHUNK);
	if (!chunk)
		return -ENOMEM;

	ret = dm_verity_sc_sha256_begin();
	if (ret)
		goto out;

	while (done < size) {
		this_size = size - done;
		if (this_size > DM_VERITY_TREE_READ_CHUNK)
			this_size = DM_VERITY_TREE_READ_CHUNK;

		ret = dm_verity_read_mmc_partition_resolved(
			runtime->source.location.mmc.dev,
			mmc, &part_info, 0, offset + done,
			chunk, this_size);
		if (ret)
			goto out;

		fed = 0;
		while (fed < this_size) {
			hash_size = this_size - fed;
			if (hash_size > SC_MAX_SIZE_PERTIME)
				hash_size = SC_MAX_SIZE_PERTIME;

			ret = dm_verity_sc_sha256_update(
				chunk + fed, hash_size, newround,
				done + fed + hash_size == size,
				digest);
			if (ret)
				goto out;

			newround = 0;
			fed += hash_size;
		}

		done += this_size;
	}

	ret = 0;
out:
	free(chunk);
	return ret;
}
#endif

static const struct dm_verity_data_ops dm_verity_data_ops = {
	.read_region = dm_verity_data_read_region,
#if defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_SECURE_SUPPORT) && \
	CONFIG_DM_VERITY_HASH_BY_SC_ENABLE
	.hash_region = dm_verity_hash_region,
#endif
};

static int dm_verity_build_bootargs(const struct dm_verity_boot_config *cfg,
				    const struct dm_verity_params *params,
				    char *bootargs,
				    unsigned int bootargs_size)
{
	char base_bootargs[256];
	int len;

	if (!cfg || !params || !bootargs)
		return -EINVAL;

	len = bootargs_mem_append(base_bootargs, sizeof(base_bootargs),
				  cfg->bootargs_base);
	if (len < 0 || len >= sizeof(base_bootargs))
		return -ENOSPC;

	return snprintf(bootargs, bootargs_size,
				"%s dm-mod.create=\"vroot,,,ro,0 %u verity 1 %u:%u %u:%u %u %u %u %u %s %s %s\"",
				base_bootargs,
				params->data_sectors,
				cfg->data_dev_major, cfg->data_dev_minor,
				cfg->hash_dev_major, cfg->hash_dev_minor,
			params->data_block_size, params->hash_block_size,
			params->data_blocks, params->hash_start_block,
			params->hash_name, params->root_hash,
			params->salt);
}

int dm_verity_boot_prepare(const struct dm_verity_boot_config *cfg,
		   struct dm_verity_boot_state *out)
{
	struct security_pubkey pubkey;
	struct dm_verity_runtime_req req;
	struct dm_verity_manifest_runtime manifest_runtime;
	struct dm_verity_data_runtime data_runtime;
	int ret;
	int len;

	if (!cfg || !out)
		return -EINVAL;

	memset(&pubkey, 0, sizeof(pubkey));
	memset(&req, 0, sizeof(req));
	memset(&manifest_runtime, 0, sizeof(manifest_runtime));
	memset(&data_runtime, 0, sizeof(data_runtime));
	memset(out, 0, sizeof(*out));

	ret = dm_verity_load_pubkey_from_source(&cfg->pubkey_source, &pubkey);
	if (ret)
		return ret;

	dm_verity_manifest_runtime_init(&manifest_runtime,
					 &cfg->manifest_source);
	memcpy(&data_runtime.source, &cfg->data_source,
	       sizeof(data_runtime.source));

	req.pubkey = &pubkey;
	req.manifest_ops = dm_verity_get_manifest_ops();
	req.manifest_priv = &manifest_runtime;
	req.data_ops = &dm_verity_data_ops;
	req.data_priv = &data_runtime;

	ret = dm_verity_runtime_run(&req, &out->runtime);
	if (ret)
		goto out_release;

	len = dm_verity_build_bootargs(cfg, &out->runtime.params,
				       out->bootargs,
				       sizeof(out->bootargs));
	if (len < 0 || len >= sizeof(out->bootargs))
		ret = -ENOSPC;
	else
		ret = 0;

out_release:
	security_pubkey_release(&pubkey);
	return ret;
}

int dm_verity_boot_run(const struct dm_verity_boot_config *cfg)
{
	struct dm_verity_boot_state state;
	int ret;

	if (!cfg)
		return -EINVAL;

	if (!cfg->kernel_bootcmd || !cfg->kernel_bootcmd[0])
		return -EINVAL;

	ret = dm_verity_boot_prepare(cfg, &state);
	if (ret)
		return ret;

	setenv("bootargs", state.bootargs);
	//printf("dmverity: bootargs=%s\n", state.bootargs);
	//printf("dmverity: verified key_id=%u tree=%.*s...\n",
	       //state.runtime.params.key_id, 8, state.runtime.params.tree_sha256);

	return run_command(cfg->kernel_bootcmd, 0);
}
