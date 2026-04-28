#ifndef __SPL_SLAVECORE_SYNC_H
#define __SPL_SLAVECORE_SYNC_H

#define SLAVECORE_SHARE_MAGIC 0x53434f52U

enum slavecore_state {
    SLAVECORE_STATE_INIT = 0,
    SLAVECORE_STATE_TX_RUNNING = 1,
    SLAVECORE_STATE_TX_DONE = 2,
    SLAVECORE_STATE_TX_FAIL = 3,
};

enum slavecore_check_result {
    SLAVECORE_CHECK_DONE = 0,
    SLAVECORE_CHECK_WAIT = 1,
    SLAVECORE_CHECK_FAIL = -1,
};

struct slave_share_mem {
    int debug;
    int rot;
    int sn_len;
    char sn[64];
    int mac_len;
    char mac[32];
    int logo_len;
    void *logo;
    volatile unsigned int magic;
    volatile unsigned int state;
};

static void spl_slavecore_share_init(struct slave_share_mem *share)
{
    unsigned int i;
    unsigned char *ptr = (unsigned char *)share;

    for (i = 0; i < sizeof(*share); ++i)
        ptr[i] = 0;

    share->magic = SLAVECORE_SHARE_MAGIC;
    share->state = SLAVECORE_STATE_INIT;
}

static int spl_slavecore_check_state(const struct slave_share_mem *share)
{
    if (!share || share->magic != SLAVECORE_SHARE_MAGIC)
        return SLAVECORE_CHECK_WAIT;

    switch (share->state) {
    case SLAVECORE_STATE_INIT:
    case SLAVECORE_STATE_TX_RUNNING:
        return SLAVECORE_CHECK_WAIT;
    case SLAVECORE_STATE_TX_DONE:
        return SLAVECORE_CHECK_DONE;
    case SLAVECORE_STATE_TX_FAIL:
        return SLAVECORE_CHECK_FAIL;
    default:
        return SLAVECORE_CHECK_WAIT;
    }
}


#endif
