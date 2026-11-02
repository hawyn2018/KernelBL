# 保留的第三方许可与来源

本包从 hnubl 2.9.2 发布源码按原样提取下列目录。版权、SPDX、许可文件和目录内的 provenance/README 记录均保留；构建使用的具体源文件由 `runtime/uefi/build_driver.py` 列出。

| 组件 | 保留位置 |
| --- | --- |
| FatFs | `third_party/fatfs/LICENSE`、源文件声明及 `README.KBL.md` |
| lwext4 | `third_party/lwext4/LICENSE` 及各源文件声明 |
| erofs-utils | `third_party/erofs-utils/COPYING`、`LICENSES/` 及源文件声明 |
| LZ4 | `third_party/lz4/LICENSE` 及源文件声明 |
| GRUB F2FS 部分 | `third_party/grub_f2fs/COPYING`、来源记录及保留的 upstream 文件 |

独立的 KBL C 移植层声明还保留在 `runtime/compat/LICENSE`。项目根许可证为 `LICENSE`；单个原始文件的版权与许可标记未因本次提取而删除或改写。
