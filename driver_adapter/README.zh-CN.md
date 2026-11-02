# KBL 驱动适配接口

本目录把平台实际回调接到原有 `kbl_native_platform`、`kbl_native_backend`
及控制器；不改 KBL 数字 ABI、文件接口、USB ABI 或原有核心源码。
新增接口代码使用 `AGPL-3.0-only`，原 runtime 与第三方文件的 SPDX、版权头及
许可证保持各自原状。

## 文件与构建依赖

- `adapter.h / adapter.c`：描述符、真实后端桥接、可选已加载 EFI service 桥接、生命周期检查。
- `board_template.c`：可编译的平台模板。内存池尚未实现，返回 NULL，明确表示未就绪。
- `tests/smoke.c / run_smoke.py`：主机烟测；没有 USB、设备或刷写动作。

集成原生后端时，将 `adapter.c` 与原有 runtime 链接；不是只拷贝一个头文件。
核心依赖为 `runtime/kbl_control.c`、`runtime/native/kbl_native.c`、
`runtime/native/kbl_efi_bootloader.c`、`runtime/kbl_elf.c`、`runtime/kbl_sha256.c`。
ARM64 原生运行还需 `runtime/native/kbl_context_switch.S` 和原有文件系统实现。
USB 桥接直接依赖 `runtime/uefi/kbl_usb.c`、`runtime/uefi/kbl_usb_native.c`。
完整 EFI 驱动的文件系统、分区、内存保护、显示及 ADB 依赖继续由
`runtime/uefi/build_driver.py` 中的原来源清单决定；不要用烟测的
`runtime/native/tests/fs_unavailable.c` 代替正式文件系统。

`board_template.c` 可替换成自己的 provider 工厂。它不被 adapter 自动调用。
NDK/LLVM 工具路径交给顶层构建入口或现有 builder 的 `--llvm-bin`，本目录不
加入设备路径、机型地址表或自动部署步骤。

## 1. 描述回调，显式启动

```c
#include "driver_adapter/adapter.h"

/* 必须持久存活且初始全零；绑定后不要复制或移动。 */
static struct kbl_driver_adapter driver;

int board_prepare(void) {
    struct kbl_driver_port port;
    int rc = kbl_board_port(&port); /* 或自己的 provider 工厂 */
    if (rc) return rc;
    return kbl_driver_init(&driver, sizeof(driver), &port, sizeof(port));
}
```

`init` 只验证、复制描述符并调用原 `kbl_native_backend` 填表，不调用 provider、
不分配内存、不读写分区、不注册 USB、不启动定时器或微内核。局部 `port`
可以离开作用域；平台结构的已知完整字段已复制到 driver。其 `opaque` 指向的
真实 provider 状态仍由调用方持有，须保留到全部使用者释放。

本 adapter 的 `abi/bytes` 与 `platform_bytes` 是独立前缀合同：拒绝不支持的 ABI、
截断必需字段、错误长度；可选字段只有完整落在可读前缀内才复制，半个指针不会
被调用，缺失字段置零。未来未知尾部被忽略。传入长度必须描述实际可读内存。

父 Fastboot 仅对明确 KBL 命令调用：

```c
int handled = kbl_driver_handle(&driver,
    (const unsigned char *)"oem kblstart", 12, &actual_output);
```

这里传控制器原有完整 `oem ...` 字节串，不是单独 `kblstart`，也不是 NUL 结尾
长度。未知普通命令返回 0，不产生输出、不读取 EL、不分配或执行后端；父环境
继续走原处理。`actual_output` 必须接原 Fastboot 真实 INFO/final 语义。
只有显式 START 走真实后端启动。没有实际 ARM64/有效 EL 或内存 provider 时
失败，不假装已经启动。

## 2. 平台能力映射

| 原生字段 | provider 必须提供的真实语义 |
| --- | --- |
| `allocate / release` | 私有、按 alignment 对齐的 RW 数据内存；精确成对释放。返回 NULL 表示分配失败。 |
| `current_el` | 实际 CurrentEL。参考函数在 AArch64 执行 `mrs CurrentEL`；其他架构返回 0（不可用），不固定报 EL1/EL2。 |
| `now_us` | 可选被动单调时钟；缺省沿用原后端 checkpoint yield，不创建新定时器。 |
| `elf.allocate/release/protect/icache` | 私有初始 RW/NX 页、真实页权限与指令缓存维护；遵循 `kbl_elf_host` 失败清理合同。空回调不视为可执行页保护已存在。 |
| `block_open / block_list` | 按真实分区 label 打开/枚举；返回原 `kbl_block`，大小与身份来自实际介质。读写、部分 I/O、flush、close 的原合同保留。 |
| `uefi_environment` | 可选真实借用 image/system table；不提供合成句柄。 |
| `efi_run / efi_run_full` | 显式 EFI 命令才运行真实 UEFI 应用，保留 UTF-16 options 与结果/资源释放合同；不在 init 执行。 |
| `display` | 显式 0 查询、1 关、2 开，返回真实 provider 结果；不用画黑屏代替硬件关屏。 |
| `poweroff` | 显式命令经原后端同步后调用真实关机；成功的关机不返回，普通 exit/destroy 不触发它。 |

可选回调不实现就留 NULL。原后端对缺失能力返回真实 unavailable/unsupported；
模板不替它们构造成功。`board_template.c` 只是编译起点，替换真实内存 provider
前不能启动。可执行程序仍按原 GPR-only、X18 保留、同 EL 协作式 ABI 运行。

## 3. USB 沿用原类型和所有权

`kbl_driver_prepare_usb` 调原 `kbl_usb_init`，接受原 `kbl_usb_ops`；
`kbl_driver_prepare_native_usb` 调原 `kbl_native_usb_init`，接受原
`kbl_native_usb_environment`。二者只准备 caller-owned、全零 session/binding，
不会 claim TX、Send、取事件、Start/Stop 控制器或更换协议槽。

后续显式 START 的观察、原 ACK terminal 完成、RX epoch、FETCH fence 与 DATA
事务必须按原 `kbl_native_usb_observe/arm_start_ack/start_ack_ready/begin` 合同
接入。prepare 本身不证明 TX idle，不授权私有 DATA。单一 native USB owner、
DMA 终态与原事件转发规则保持不变。

`kbl_firmware_link`、`kbl_usb_parent_facts`、ADB USB 环境和 service 类型直接
使用原头文件；没有新造一套同名布局。父 facts 必须来自当前父映射的功能与
数据流解析；标准结构偏移是 ABI，具体 RVA 是当前输入，不是机型列表。

## 4. 可选：桥接已经加载的 EFI service

如果使用原 `kbl_driver.efi`，在自己拥有的返回 image handle 上获取其
`kbl_service_protocol`，再调用 `kbl_driver_bind_service`。不要全局 Locate 一个
可能陈旧或无关的 service。bind 验证 magic/ABI/长度，只记录描述符，不调用
service；原 service 和 link/facts 的底层状态须保持存活。

显式 `kbl_driver_firmware_request` 直接调用已有 service `request`。只有
START/ADB 操作才在需要时先调原 `bind_usb(parent_facts)`；缺失可选 bind hook
返回 unsupported，不盲目猜测地址。service 模式下，本地 `handle/backend`
入口拒绝使用，避免创建两个世界。provider 必须串行化这些入口；此薄桥不
取代原 ADB 多会话执行队列或固件事件循环。

## 5. 停止、回读与释放

- 运行期间保留 adapter、provider opaque、借用的 USB session/binding 和 service。
- 先使用原显式 OFF/停止流程并处理返回；有 reader/snapshot、会话、挂载或 I/O
  租约时，原 backend 会拒绝过早释放，adapter 不绕过它。
- USB 超时不等于 DMA 已释放，必须继续原 reap/终态/转发流程；不要因超时就 free。
- `kbl_driver_can_destroy` 只在本地 context/upload 为空、USB quiescent、已使用的
  service 明确 can_unload 时返回 1。service 没有 can_unload 时保留对象。
- `kbl_driver_destroy` 不发送 OFF、abort、关机或重试写；忙时返回 `KBL_AGAIN`，
  原对象继续有效。成功仅清除桥的状态，不释放 caller-owned provider。
- adapter 含指向自身 platform 的 backend 指针，禁止 memcpy 后当作独立实例。

## 主机烟测与范围

```text
python driver_adapter/tests/run_smoke.py --output PATH_OUTSIDE_SOURCE
```

该测试链接真实 adapter、controller、native backend、ELF 与 USB 实现。
主机没有 ARM64 特权环境，明确验证 START 报错而不是模拟启动。其 FS provider
使用已有测试用 unsupported 实现，USB/service 是独立计数模型，检查 bind/未知
命令零 provider I/O、前缀/可选字段、实际原函数转发、原生启动失败、USB 槽未改、
忙时保留、START 才绑定父 facts、缺失 hook 不冒充成功等。

这不代表特定手机的内存属性、USB、显示、EFI 引导、分区或电源已适配；本目录
没有设备测试或自动刷写步骤。
