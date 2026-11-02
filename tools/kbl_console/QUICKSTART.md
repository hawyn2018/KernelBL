# KernelBL Fastboot Shell

[中文版](#中文) · [English](#english) · [KernelBL](https://github.com/hawyn2018/KernelBL)

<a id="中文"></a>
## 中文

本包是 KernelBL 的电脑端包装器，需要 **Python 3**、原生 **Android platform-tools**，以及已接入 KBL 的设备固件。Python 和原生 Fastboot 不在这个 ZIP 中。

解压 `run_kbl_fastboot_shell.zip`，在该目录打开终端。PowerShell：

```powershell
./fastboot.cmd --kbl-native "C:\platform-tools\fastboot.exe" -s SERIAL oem kblstart
```

CMD 把 `./fastboot.cmd` 写成 `fastboot.cmd`。将原生工具路径与 `SERIAL` 换成实际值；只有一台设备时可省略 `-s SERIAL`。包装器自有 `--kbl-*` 参数应放在原生 Fastboot 参数之前。也可直接运行 Python：

```text
python kbl_console.py --kbl-native FASTBOOT_EXECUTABLE -s SERIAL oem kblstart
```

`kblstart` 启动 KBL 并进入 `# ` 交互台；`kblin` 重新接管已有环境。`exit`、Ctrl+C 和 `:detach` 仅断开电脑端。显式 `oem kbloff` 停止环境。显式启用 ADB 使用 `oem kbladb`，随后 `adb shell`；仅 `kblstart` 不主动启用 ADB。

```text
:help
:status
mkdir /tmp
:push "LOCAL_FILE" /tmp/example.bin
sha256sum /tmp/example.bin
:pull /tmp/example.bin "NEW_LOCAL_FILE"
:pullpart PARTITION_NAME "NEW_LOCAL_PARTITION_IMAGE"
:detach
```

环境与数据受设备提供者和实际内存约束。回读目标文件应不存在；设备需实际提供二进制 fetch。命令失败/断连不自动重放写操作。`efi`、挂载、SDK 程序和驱动移植用法见 [完整说明](https://github.com/hawyn2018/KernelBL#中文)。

<a id="english"></a>
## English

This is the PC-side KernelBL wrapper. Install **Python 3** and native **Android platform-tools** separately. Your device firmware must already provide the KernelBL integration. This ZIP does not install a device runtime.

Extract the ZIP and open a terminal in its directory. On Windows PowerShell:

```powershell
./fastboot.cmd --kbl-native "C:\platform-tools\fastboot.exe" -s SERIAL oem kblstart
```

In CMD, use `fastboot.cmd` without `./`. Replace the executable path and serial; omit `-s SERIAL` for a single device. On other hosts, or directly with Python:

```text
python kbl_console.py --kbl-native FASTBOOT_EXECUTABLE -s SERIAL oem kblstart
```

Place wrapper `--kbl-*` options before native Fastboot arguments. `kblstart` starts the environment and attaches the console; `kblin` reattaches. `exit`, Ctrl+C and `:detach` retain the environment. `oem kbloff` explicitly stops it. Enable ADB separately with `oem kbladb`, then use `adb shell`.

Use `:help`, `:status`, `:push LOCAL /tmp/file`, `:pull /tmp/file NEW_LOCAL` and `:pullpart LABEL NEW_LOCAL`. Pull requires a fresh destination and real device fetch support. Transport failure does not automatically replay writes. See the [full English documentation](https://github.com/hawyn2018/KernelBL#english).

## Source and license / 源码与许可证

Version: KernelBL **1.0.0**; protocol: **KBL1**. Source: [hawyn2018/KernelBL](https://github.com/hawyn2018/KernelBL/tree/v1.0.0/tools/kbl_console). The included `LICENSE` is the project AGPLv3 text; original source-file notices remain applicable. Protocol details: `KBL_PROTOCOL.md`.

QQ群：[941336897](https://qm.qq.com/q/8ax2kPeghy) · Telegram：[blroot](https://t.me/blroot)
