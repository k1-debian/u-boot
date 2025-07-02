
#ifndef __SPL_READ_RESERVED_H__
#define __SPL_READ_RESERVED_H__

#include "spl_ota_utils.h"

struct ota_ops;
/**
 * 从预留区域读取 MAC/SN 等信息，并拼接到 cmdargs 中
 *
 * @param ota_ops     - flash 操作接口
 * @param type_str    - 字符串标识，如 "mac" 或 "sn"
 * @param offset_kb   - 相对于 nand_size 的偏移大小（单位 KB）
 * @param tmp_buf     - 外部传入的静态缓冲区，用于拼接字符串
 * @param buf_size    - 缓冲区大小
 * @param cmdargs     - 输入输出参数：原始命令参数，更新后指向 tmp_buf
 *
 * @return int        - 成功返回 0，失败返回 -1
 */
int spl_read_reserved(
    struct ota_ops *ota_ops,
    const char *type_str,
    uint32_t offset_kb,
    char *tmp_buf,
    size_t buf_size,
    const char **cmdargs);

#endif /* !SPL_READ_RESERVED_H */
