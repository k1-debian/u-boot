# Commit Companion Note
## 提交说明

本文概述了已落地的公共安全启动链路、模块职责与复用边界。

本次提交旨在重构并建立标准化的公共安全启动链路，对启动模式选择、安全校验、可信公钥加载、Manifest 获取及底层介质访问等能力进行分层治理，形成一套通用且可复用的实现方案。相较于单一板级的适配，本次整理的核心目标是将安全链路模块化解耦，并借助统一的 Source/Config 装配方法，构建出一条完整、规范的安全校验启动路径。

当前公共安全链路可以概括为：

- `bootroom-help` stage1 负责安全拉起 stage2 SPL
- stage2 SPL 负责 recovery / normal 启动选择，并在 normal 路径加载完整 U-Boot
- 完整 U-Boot 以 `dmverity` 作为安全校验启动入口
- `dm-verity` 负责 manifest 验签、verity tree 校验、bootargs 生成和 kernel 启动
- `keybox` 负责可信公钥对象加载与安全对象访问
- 板级 helper 负责把 pubkey source、manifest source、data source 和 kernel bootcmd 装配成板级实现

## 当前公共安全链路

```text
NOR bootroom
  -> stage1 helper SPL
       -> 初始化最小硬件
       -> 从 eMMC 固定偏移加载 stage2 SPL
       -> secure_scboot(stage2 spl) [if enabled]
       -> 跳转到 stage2 SPL

stage2 SPL
  -> 读取 GPT / NV / OTA 状态
  -> recovery 命中
       -> 直接加载 recovery image
  -> normal 路径
       -> 直接加载 raw U-Boot

full U-Boot
  -> bootcmd = dmverity
  -> board_dm_verity_get_config()
  -> dm_verity_boot_run()
       -> 加载 pubkey
       -> 加载 manifest
       -> 校验 manifest 签名
       -> 校验 verity tree 摘要
       -> 生成最终 bootargs
       -> run_command(kernel_bootcmd)
```

## 公共安全链路的模块职责

### 1. `bootroom-help` stage1

职责：

- 提供 NOR 首阶段 helper 入口
- 完成 stage2 SPL 启动前所需的最小化初始化
- 从 eMMC 固定偏移读取 stage2 SPL
- 在 secure 场景下完成 stage2 SPL 解包并跳转执行

对应文件：

- `arch/mips/cpu/xburst2/x2600/bootroom_help.c`
- `include/x2600_bootroom_help.h`

### 2. stage2 SPL OTA / normal 选择

职责：

- 读取 `nv` 分区和 recovery 标记
- 命中 recovery 时加载 recovery image
- normal 路径下加载 raw U-Boot，把后续安全校验链路切换到完整 U-Boot

对应文件：

- `common/spl/spl_ota_jzsd.c`
- `common/spl/spl_jzsdhci.c`

其中关键路径是：

- `spl_jzsd_load_normal_image()` 在 `CONFIG_BOARD_DM_VERITY_ENABLE` 打开时调用 `jzsd_load_uboot()`，把 normal 路径切到完整 U-Boot 的公共安全校验链路

### 3. `dm_verity_boot`

职责：

- 作为完整 U-Boot 中的公共安全校验启动入口
- 获取板级 `dm-verity` 装配配置
- 组织 pubkey source / manifest source / data source
- 调用 runtime 完成 manifest 验签和 tree 校验
- 生成最终 `bootargs`
- 调用板级 `kernel_bootcmd`

对应文件：

- `common/cmd_dmverity.c`
- `drivers/verity/dm_verity_boot.c`
- `include/dm_verity_boot.h`

### 4. `dm_verity_runtime`

职责：

- 读取 manifest 与签名
- 解析 manifest 存储格式
- 执行 manifest 签名校验
- 执行 manifest 指定的 verity tree 区间摘要校验
- 导出最终运行参数

对应文件：

- `drivers/verity/dm_verity_runtime.c`
- `include/dm_verity_runtime.h`

### 5. `manifest / media / pubkey source` 公共适配层

职责：

- 屏蔽 manifest 布局差异
- 屏蔽底层介质读取差异
- 屏蔽可信公钥来源差异

对应文件：

- `drivers/verity/dm_verity_manifest_source.c`
- `drivers/verity/dm_verity_media.c`
- `drivers/verity/dm_verity_pubkey_source.c`

当前已经接通的公共路径是：

- manifest source:
  - `DM_VERITY_MANIFEST_SOURCE_ROOTFS_TAIL`
  - `DM_VERITY_MANIFEST_SOURCE_MMC_RAW`
- data source:
  - `MMC`
- pubkey source:
  - `KEYBOX`

### 6. `security_pubkey`

职责：

- 解析 RSA 公钥存储格式
- 提供 RSA2048 + SHA256 签名校验能力
- 提供公钥深拷贝和释放能力，保证安全链路中的对象生命周期正确

对应文件：

- `drivers/verity/security_pubkey.c`
- `include/security_pubkey.h`

当前 `dm_verity_pubkey_source.c` 在 `keybox_load_pubkey_from_source()` 返回后调用 `security_pubkey_dup()`，确保公共安全链路在 `keybox_close()` 之后仍持有有效公钥数据。

### 7. `keybox`

职责：

- 打开 keybox 容器
- 解析对象表
- 按 selector 查找对象
- 处理容器解包、对象解密和 trust check
- 将可信对象导出为 `security_pubkey`

对应文件：

- `drivers/keybox/keybox.c`
- `drivers/keybox/keybox_source.c`
- `drivers/keybox/keybox_source_mmc.c`
- `drivers/keybox/keybox_source_nor.c`
- `drivers/keybox/keybox_source_nand.c`
- `include/keybox.h`

## keybox 在公共安全链路中的处理流程

```text
dm_verity_load_pubkey_from_source()
  -> keybox_load_pubkey_from_source()
    -> keybox_open_source()
      -> keybox_open()
        -> 读取 header
        -> outer SCBOOT 解包 [if enabled]
        -> 解析 container / object table
    -> keybox_verify()
    -> keybox_find_object()
      -> USERKEY0 解密对象 [if enabled]
      -> NKU trust check [if enabled]
    -> keybox_object_to_pubkey()
  -> security_pubkey_dup()
```

P2091 当前启用的相关能力：

- `CONFIG_KEYBOX_OUTER_SCBOOT_ENABLE`
- `CONFIG_KEYBOX_AES_USERKEY0_ENABLE`
- `CONFIG_KEYBOX_NKU_CHECK_ENABLE`

## P2091 板级接入实现

P2091 当前接入配置集中在：

- `include/configs/p2091_printer.h`
- `board/ingenic/x2600h_halley7/dmverity.c`
- `board/ingenic/x2600h_halley7/board.c`

P2091 当前通过板级装配方式把这套公共安全链路接入到实际启动流程中，具体策略是：

- recovery 在 SPL 阶段处理
- normal 路径进入完整 U-Boot
- `CONFIG_BOOTCOMMAND` 配置为 `dmverity`
- `board_dm_verity_get_config()` 由板级 helper 组装总配置
- manifest 公钥从 `riscv0` 分区内 keybox 中按对象名读取
- manifest 从 `rootfs` 分区尾部 slot 读取
- verity tree 按 manifest 给出的 `part_name/tree_offset/tree_size` 从 MMC 读取
- kernel 最终由板级 `mmc read ...; bootm ...` 启动

这说明前面定义的公共模块和装配方法，已经在 P2091 上形成了完整的板级落地实现。

对应调用路径为：

```text
CONFIG_BOOTCOMMAND = "dmverity"
  -> common/cmd_dmverity.c
    -> board_dm_verity_get_config()
      -> board_dm_verity_helper_get_config()
    -> dm_verity_boot_run()
      -> dm_verity_boot_prepare()
        -> dm_verity_load_pubkey_from_source()
        -> dm_verity_runtime_run()
        -> dm_verity_build_bootargs()
      -> setenv("bootargs", ...)
      -> run_command(CONFIG_P2091_NORMAL_BOOTCMD)
```

## 当前公共能力范围

- X2600 `bootroom-help` stage1 -> stage2 SPL from eMMC
- stage2 SPL recovery OTA / normal 分流
- normal 路径下加载完整 U-Boot
- `dmverity` 命令式安全校验启动入口
- keybox-backed pubkey 加载
- manifest 签名校验
- verity tree SHA256 校验
- `ROOTFS_TAIL` manifest 布局
- MMC 分区名查找和按偏移读取
- `dm-mod.create=` bootargs 拼接

## 当前可复用边界与注意点

1. `rollback_version` 当前只解析和导出  
   `dm_verity_runtime_run()` 会把它带到 `runtime_result`，当前 U-Boot 侧未执行防回滚策略。

2. `ROOTFS_TAIL` 依赖固定镜像布局  
   rootfs 尾部需要存在固定格式的 tail + slot 结构，镜像制作侧必须严格匹配。

3. 当前实际数据路径是 MMC  
   keybox source、manifest source、data source 当前实际接通和验证的公共路径都是 MMC。

4. NOR/NAND source/data 路径未实现  
   相关接口已定义，但当前没有实际 backend 实现。

5. `DIRECT pubkey` 板级路径未接通  
   通用 source 类型已定义，当前板级装配层只接 `KEYBOX`。

6. secure 相关能力依赖现有 SC 机制  
   包括 stage2 SPL 解包、keybox outer unwrap、对象 trust check、可选 hash offload。

## 一句话总结

当前代码已经落地的是一套围绕整条安全链路组织起来的公共实现方法：上层用统一装配方式串起 `bootroom-help`、stage2 SPL、`dmverity`、`keybox` 和介质访问，下层再由 P2091 板级把这套公共能力落地为可运行的安全启动实现。
