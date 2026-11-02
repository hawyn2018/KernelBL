# Changelog / 更新记录

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
