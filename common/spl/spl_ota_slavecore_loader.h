#ifndef __SPL_OTA_SLAVECORE_LOADER_H__
#define __SPL_OTA_SLAVECORE_LOADER_H__

struct spl_ota_slavecore_ops {
    void *priv;
    int (*load)(void *priv, unsigned int src, unsigned int len, unsigned long dst);
    void (*flush_cache_all)(void *priv);
    unsigned int (*get_reset)(void *priv);
    void (*set_reset)(void *priv, unsigned int value);
    void (*set_reset_entry)(void *priv, unsigned int value);
};

int spl_ota_slavecore_load_and_start(
    const struct spl_ota_slavecore_ops *ops,
    unsigned int image_offset,
    unsigned long load_addr,
    int cpu);

#endif
