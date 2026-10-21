# PPC：STM32H743 综合图形工程

基于 STM32H743IIT6 的综合图形工程，包含 LVGL 界面、双通道示波器、DAC 波形输出、FFT 频谱和现有外设应用。

本目录是 2026-10-21 从当前 PPC 工程整理的独立发布副本，用户已确认可以发布，并补全了 SD 卡资源。

## 固件与完整 SD 卡资源下载

在 [2026-10-21 发布页](https://github.com/XboxNahida/-STM32H743-7-/releases/tag/ppc-2026-10-21) 的 Assets 下载：

- H743_LCD_Keil5.hex：完整固件。
- SD-card-resources-20261021.zip：用户补全的全部 SD 卡目录，解压内容复制到 SD 卡根目录。
- SHA256SUMS.txt：固件、SD 资源包及文件清单的 SHA256。
- SD-card-files-20261021.sha256：SD 资源包内逐文件校验清单。

仓库为私有，需要具有仓库访问权限的 GitHub 账户登录后下载。源码也可通过发布页的 Source code 或仓库 Code → Download ZIP 获取。

## 目录

| 路径 | 内容 |
| --- | --- |
| Core/ | 应用、外设、HAL 集成及中间件源码 |
| Drivers/、Utilities/ | STM32 HAL、CMSIS 和 JPEG 等构建依赖 |
| lvgl/、lvgl_GUI/ | 当前实际使用的 LVGL 及图形界面源码 |
| MDK-ARM/ | Keil 工程、启动汇编、RTE 头文件及自定义链接脚本 |
| H743_LCD_Keil5.ioc | CubeMX 配置；重新生成前请阅读恢复指南 |
| Tests/、Tools/ | 主机测试及字体、图像、视频等转换工具源码 |
| Docs/ | 使用、硬件、引脚、内存、CubeMX 恢复及稳定版技术资料 |
| Firmware/ | 本次待验收固件、SHA256、构建日志与验证结果 |

## 编译

1. 安装 Keil MDK，使用 **ARM Compiler 5.06 update 7 build 960**，并安装 STM32H743 所需器件支持包。
2. 打开 MDK-ARM/H743_LCD_Keil5.uvprojx，选择 H743_LCD_Keil5 目标，执行 Rebuild。
3. 生成的 HEX 位于 MDK-ARM/H743_LCD_Keil5/H743_LCD_Keil5.hex；该目录的编译中间文件不应提交。

全部参与编译的源码与链接脚本随目录保留，无需待移植功能文件目录。Tools/lv_font_conv-master 的 Node 依赖未随包携带；只有重新生成字体时才需按其 package.json 安装，Keil 构建不依赖 node_modules。主机回归需要 GCC，入口为 Tools/验证示波器.ps1 和 Tools/验证示波器界面.ps1；GCC 使用含中文的绝对输出路径时可能失败，建议把发布包解压到仅含 ASCII 的路径运行。

## 烧录与外置资源

Firmware/ 下的 H743_LCD_Keil5.hex 为本次独立重编译的发布版本；Docs/稳定版/2026-10-20-示波器/ 保留此前稳定归档。校验文件和构建结果以 Firmware/说明.md 为准。

完整 SD 卡资源以 Release 的 SD-card-resources-20261021.zip 为准；仓库保留部分字体、图片和界面资源，不能用仓库内的局部目录代替完整资源包。HEX 仅包含 MCU 内部固件，不代替 QSPI/NAND/SD 卡数据备份。请先备份卡内已有数据，再按原有目录复制，不要擦除已有外置存储。

使用与验收请阅读 [示波器说明](Docs/H743示波器使用与验收.md)、[工程文档索引](Docs/README.md)、[CubeMX 恢复指南](Docs/CubeMX恢复指南.md) 和 [发布范围](Docs/发布范围说明.md)。

## 发布范围

保留当前源码、实际构建依赖、必要工具、技术说明、原理图及可下载固件。待移植文件、计划、调试进度、缓存和 IDE 个人布局不进入发布。用户补全的 SD 测试影音、ROM、记录和目录内容完整包含在独立 SD 资源包中，本地原件保留。

这是单独整理的当前工程快照，未带入原仓库的历史对象。第三方代码和资源沿用原有版权说明；没有为整个工程擅自增加开源许可证。
