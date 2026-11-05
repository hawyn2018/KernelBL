# Changelog / 更新记录

## 1.1.0 — 2026-11-06

### 中文

- 同步 hnubl 3.0.2 引入并保留在 3.0.5 的 KBL 运行时更新。KBL ABI 1 和现有 Fastboot 包装器协议保持兼容。
- 新增 `efi -cs` / `efi --clearstart`。启动目标 EFI 前排空本次任务输出、清理 KBL 会话和资源，停止旧 USB 接收，并由独立 EFI 载入器完成交接。可以与 `-bl` 组合。
- 改进 ADB 兼容性，处理旧版 Windows ADB 的 USB 读取，并在 EFI 交接时等待接收完成、恢复 Fastboot USB 描述符。
- 新增 `clear errorcount`，用于清除并回读工具私有的启动失败计数。
- 手动 `fastboot continue` 调用的 `BootLinux` 返回时，保留原生返回状态和清理流程。
- Release 分开提供完整源码与 `run_kbl_fastboot_shell.zip`，并附 SHA-256 校验文件。

### English

- Syncs the KBL runtime changes introduced in hnubl 3.0.2 and retained in 3.0.5. The KBL ABI and Fastboot wrapper protocol remain compatible at version 1.
- Adds `efi -cs` / `efi --clearstart`. This drains job output, cleans up KBL sessions and resources, stops old USB receive activity, and transfers control to a separate EFI loader. It can be combined with `-bl`.
- Improves USB compatibility with legacy Windows ADB and restores Fastboot descriptors after pending receive operations finish during EFI handoff.
- Adds `clear errorcount` to clear and verify the private boot-failure record.
- Preserves the original return status and cleanup if `BootLinux` returns from manual `fastboot continue`.
- Release assets include complete source, a separate `run_kbl_fastboot_shell.zip`, and SHA-256 checksums.

## 1.0.0 — 2026-11-03

### 中文

- 首次以 **KernelBL** 独立项目发布，基于 hnubl 2.9.2 的 KBL 源码快照。
- 包含同 EL ARM64 协作式运行时、UEFI 驱动、Fastboot 控制台、显式启用的 ADB、多会话、RAMFS/VFS、文件系统与分区读写接口、原生程序 SDK 和 EFI 应用入口。
- 提供独立构建入口和带 ABI/长度契约的驱动适配接口、板级模板及主机验证。
- 加入浅粉色矢量标志、中英双语功能介绍及使用文档。
- KBL 原有数字 ABI 1 保持；此次发布没有修改运行时 C、头文件、汇编或第三方实现。
- 文档修正了旧容量说明和 ADB 启用示例，具体差异可由来源清单核对。

### English

- First standalone **KernelBL** release, extracted from the KBL source snapshot in hnubl 2.9.2.
- Includes the same-EL cooperative ARM64 runtime, UEFI driver, Fastboot console, explicitly enabled ADB, multiple sessions, RAMFS/VFS, filesystem and partition interfaces, native program SDK, and EFI application launcher.
- Provides standalone build commands, a driver adapter with ABI/size contracts, a board template, and host validation.
- Adds the light rose vector identity and bilingual feature and usage documentation.
- Retains numeric KBL ABI 1. Runtime C, headers, assembly and third-party implementation files are unchanged by this publication.
- Corrects stale capacity notes and the ADB activation example in documentation; provenance manifests describe the source snapshot and changes.
