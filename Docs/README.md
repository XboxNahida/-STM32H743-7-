# PPC 项目文档

> STM32H743IIT6 + LVGL GUI + QSPI NOR Flash + SD 卡

> 2026-10-21 发布副本入口：当前固件与完整重编译结果见 ../Firmware/说明.md，范围见 发布范围说明.md。本页保留历史技术资料；外置 Flash 型号及功能现状请结合稳定版记录和实际硬件核对。

---

## 项目简介

基于正点原子阿波罗 STM32H743IIT6 开发板的综合 GUI 项目，使用 LVGL 图形库，1024×600 RGB565 LCD 屏幕（GT9xxx 触摸）。

## 硬件资源

| 资源 | 型号/容量 |
|------|----------|
| MCU | STM32H743IIT6 |
| SDRAM | 32MB (0xC0000000) |
| NOR Flash | W25Q256JV 32MB (QSPI) |
| LCD | 1024×600 RGB565, GT9xxx 触摸 |
| SD 卡 | SDMMC1 4-bit 模式, 37.5MHz |

## 内存分配

| 区域 | 地址 | 容量 | 用途 |
|------|------|------|------|
| 内部 Flash | 0x08000000 | 2MB | 代码 + 字体数据 |
| DTCM | 0x20000000 | 128KB | 栈 + 堆 + 高速变量 |
| AXI SRAM | 0x24000000 | 512KB | LVGL 缓冲(120KB) + 通用数据 |
| SDRAM | 0xC0000000 | 32MB | LCD 帧缓冲 + LVGL内存池(512KB@0xC0500000) + 壁纸/图片缓存 + 视频缓冲 |
| QSPI Flash | 0x90000000 | 32MB | 字库(内存映射) + 图片存储 |

## 文档索引

| 文档 | 说明 |
|------|------|
| `IO口配置文档.md` | 当前 CubeMX 引脚配置 |
| `CubeMX恢复指南.md` | CubeMX 重新生成后需手动恢复的修改 |
| `LVGL 配置参考手册.md` | LVGL 配置项速查 |
| `内存分析_与正点原子对比.md` | 内存占用分析 |
| `SD卡必备文件/` | SD 卡文件结构参考 |
| `MDK配置备份/` | Keil 工程配置备份 |

## SD 卡文件结构

```
SD 卡根目录:
  SYSTEM/
    IMG/
      _PPC_lock_RGB565_1024x600.bin   ← 壁纸 (1024×600 RGB565)
      _1_RGB565_285x107.bin           ← 图片1
      _2_RGB565_700x370.bin           ← 图片2
      _3_RGB565_600x220.bin           ← 图片3
    FONT/
      lv_font_myyh_12.bin
      lv_font_myyh_14.bin
      lv_font_myyh_16.bin
      lv_font_myyh_18.bin
      lv_font_myyh_20.bin
      lv_font_myyh_30.bin
```

> 首次上电或 QSPI 擦除后，系统会自动从 SD 卡拷贝 `.bin` 文件到 QSPI Flash。

## QSPI Flash 分区

| 偏移 | 大小 | 内容 |
|------|------|------|
| 0x000000 | 1.2MB | 壁纸 (1024×600 RGB565) |
| 0x140000 | ~60KB | 图片1 |
| 0x160000 | ~506KB | 图片2 |
| 0x220000 | ~258KB | 图片3 |
| 0x700000 | 1MB | myyh_16 字体 (内存映射) |

## 快速开始

1. 用 Keil MDK 打开 `MDK-ARM/H743_LCD_Keil5.uvprojx`
2. 编译下载
3. 首次运行需 SD 卡包含 `SYSTEM/` 目录下的 `.bin` 文件

## 已知问题

### SDRAM CAS Latency 不匹配导致 LVGL 内存池崩溃 [已修复 2026-06-17]

**现象**：LVGL 内存池放入 SDRAM 后 `lv_init()` 触发 HardFault (IMPRECISERR) 或看门狗复位。

**根因**：`fmc.c` 中 `SDRAM_Initialization_Sequence()` 将 SDRAM 芯片编程为 CAS=2，但 HAL 按 CAS=3 配置 FMC 时序寄存器。1 时钟周期的时序错位导致 TLSF 密集链表操作时数据损坏。

**修复**：`SDRAM_MODEREG_CAS_LATENCY_2` → `SDRAM_MODEREG_CAS_LATENCY_3`，统一为 CAS=3。



### SD 卡/JPEG/AVI AHB3 总线竞争 [已修复]

**现象**：
- 相册快速切图时 SD 卡读取失败：`[SD] Read error`
- AVI 视频播放出现大量 `failed`（JPEG 解码超时）
- SD 卡仅工作在 1-bit 模式

**根因**：
JPEG 硬解码 MDMA 通道与 SDMMC1 共用 AHB3 总线矩阵，数据搬运时产生竞争。
JPEG 输入缓冲 `g_file_content` 位于 AXI SRAM (AHB3)，与 SDMMC1 直接冲突。

**最终方案**（3 个 commit 联合修复）：

| 修复 | 文件 | 内容 |
|------|------|------|
| SD 4bit + 降频 | `sdmmc.c` | BusWide 1B→4B, ClockDiv 0→2 (37.5MHz) |
| 缓冲移入 SRAM1/2 | `events_init.c` + `.sct` | g_file_content 192KB → AHB2 总线 |
| JPEG 超时提前退出 | `mjpeg.c` | FINISHED 且差距≤16行时 break |
| LV_MEM_SIZE 优化 | `lv_conf.h` | 256→160KB + 32KB 动态扩展 |

**内存分配（最终）：**

| 区域 | 容量 | 用途 | 总线 |
|------|------|------|------|
| DTCM | 128KB | 栈+堆+全局 | TCM |
| AXI SRAM | 512KB | LVGL+HAL+驱动 | AHB3 |
| SRAM1 | 192KB | AVI/JPEG 帧缓冲 | AHB2 ✓ |
| SRAM2 | 64KB | .ANY 溢出 | AHB2 ✓ |

> 详细排障过程见 `Docs/SD卡4bit与AHB3竞争-排障全记录.md`。
