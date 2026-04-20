#include "spl_ota_slavecore_loader.h"

#include <errno.h>

int spl_ota_slavecore_load_and_start(
    const struct spl_ota_slavecore_ops *ops,
    unsigned int image_offset,
    unsigned long load_addr,
    int cpu)
{
    unsigned int image_len;
    unsigned int reset;
    int ret;

    if (!ops || !ops->load || !ops->flush_cache_all || !ops->get_reset ||
        !ops->set_reset || !ops->set_reset_entry)
        return -EINVAL;

    ret = ops->load(ops->priv, image_offset, 2048, load_addr);
    if (ret)
        return ret;

    image_len = *(unsigned int *)(load_addr + 12);
    ret = ops->load(ops->priv, image_offset, image_len, load_addr);
    if (ret)
        return ret;

    ops->flush_cache_all(ops->priv);
    ops->set_reset_entry(ops->priv, (unsigned int)(load_addr & 0x1fffffffU));
    reset = ops->get_reset(ops->priv);
    reset |= 1U << cpu;
    ops->set_reset(ops->priv, reset);
    reset &= ~(1U << cpu);
    ops->set_reset(ops->priv, reset);

    return 0;
}
