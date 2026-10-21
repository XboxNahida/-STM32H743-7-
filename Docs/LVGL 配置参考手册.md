# LVGL 配置参考手册

## 文件概览

| 文件 | 作用 |
|------|------|
| `lvgl/lv_conf.h` | LVGL 主配置文件（所有模块、调试、内存） |
| `lvgl_GUI/custom/lv_conf_ext.h` | 本板自定义覆盖配置 |
| `lvgl/porting/lv_port_disp.c` | 显示驱动常量（分辨率、缓冲区大小） |

---

## 1. 显示与帧率

| 文件 | 行号 | 宏 | 当前值 | 作用 |
|------|------|-----|--------|------|
| `lv_conf.h` | 30 | `LV_COLOR_DEPTH` | `16` | 颜色格式：RGB565 |
| `lv_conf.h` | 91 | `LV_DEF_REFR_PERIOD` | `16` | 刷新周期 16ms => 最大 ~62.5 FPS |
| `lv_conf.h` | 95 | `LV_DPI_DEF` | `130` | 默认 DPI，影响控件默认尺寸 |
| `lv_port_disp.c` | 24 | `MY_DISP_HOR_RES` | `1024` | 屏幕宽度（像素） |
| `lv_port_disp.c` | 29 | `MY_DISP_VER_RES` | `600` | 屏幕高度（像素） |
| `lv_port_disp.c` | 32 | `BYTE_PER_PIXEL` | `2` | 每像素字节数（RGB565 = 2） |
| `lv_port_disp.c` | 35 | `BUF_LINES` | `60` | LVGL 局部缓冲行数 = 60行 = 120KB |

**帧率调优：**
- 减小 `LV_DEF_REFR_PERIOD` 可提高帧率上限（如 `5` => 最大 200 FPS）
- 减小 `BUF_LINES` 可降低内存占用，但渲染次数增多、帧率下降
- 增大 `BUF_LINES` 可减少渲染次数、提高帧率（如 `100` => 200KB）

---

## 2. 内存配置

| 文件 | 行号 | 宏 | 当前值 | 作用 |
|------|------|-----|--------|------|
| `lv_conf.h` | 72 | `LV_MEM_SIZE` | `256 * 1024` (256KB) | LVGL 内部堆大小 |
| `lv_conf.h` | 78 | `LV_MEM_ADR` | `0` | 0=自动分配；可设为固定地址（如 SDRAM） |
| `lv_conf.h` | 132 | `LV_DRAW_BUF_STRIDE_ALIGN` | `1` | 行跨度对齐（字节） |
| `lv_conf.h` | 135 | `LV_DRAW_BUF_ALIGN` | `4` | 缓冲区起始对齐 |
| `lv_conf.h` | 149 | `LV_DRAW_LAYER_SIMPLE_BUF_SIZE` | `32 * 1024` | 图层混合缓冲（32KB） |
| `lv_conf.h` | 155 | `LV_DRAW_LAYER_MAX_MEMORY` | `0` | 0=不限制图层内存 |
| `lv_conf.h` | 160 | `LV_DRAW_THREAD_STACK_SIZE` | `8 * 1024` | 渲染线程栈（无 OS 时无效） |
| `lv_conf.h` | 110 | `LV_USE_OS` | `LV_OS_NONE` | 无操作系统 => 单线程 |

**内存调优：**
- 如果控件/字体/图片装不下，增大 `LV_MEM_SIZE`
- 将 `LV_MEM_ADR` 设为 `0xC0000000` 可把 LVGL 堆搬到外部 SDRAM
- 内存紧张时可减小 `LV_DRAW_LAYER_SIMPLE_BUF_SIZE`（会影响混合性能）

---

## 3. 渲染加速

| 文件 | 行号 | 宏 | 当前值 | 作用 |
|------|------|-----|--------|------|
| `lv_conf.h` | 171 | `LV_USE_DRAW_SW` | `1` | 软件渲染（必须开启的基线） |
| `lv_conf.h` | 396 | `LV_USE_DRAW_DMA2D` | `0` | LVGL 内置 DMA2D 渲染引擎（此处关闭） |
| `lv_conf_ext.h` | 28 | `LV_USE_DMA2D_FLUSH` | `1` | **本板自定义**：DMA2D 刷新。1=DMA2D，0=CPU逐行拷贝 |

**注意：** `LV_USE_DRAW_DMA2D` 是 LVGL 自带的 DMA2D 渲染单元（关闭，因为与自定义 flush 冲突）。
本项目的 `LV_USE_DMA2D_FLUSH` 仅加速最终的显存拷贝操作。

---

## 4. 调试与日志

### 4.1 日志系统

| 文件 | 行号 | 宏 | 当前值 | 作用 |
|------|------|-----|--------|------|
| `lv_conf.h` | 460 | `LV_USE_LOG` | `0` | 日志总开关。设为 `1` 启用 |
| `lv_conf.h` | 469 | `LV_LOG_LEVEL` | `LV_LOG_LEVEL_WARN` | 级别：TRACE/INFO/WARN/ERROR/USER/NONE |
| `lv_conf.h` | 473 | `LV_LOG_PRINTF` | `0` | 0=自定义回调输出；1=用 printf() |
| `lv_conf.h` | 483 | `LV_LOG_USE_TIMESTAMP` | `1` | 日志带时间戳 |
| `lv_conf.h` | 486 | `LV_LOG_USE_FILE_LINE` | `1` | 日志带源文件名:行号 |

### 4.2 跟踪子模块（仅 LV_USE_LOG=1 时生效）

| 文件 | 行号 | 宏 | 当前值 | 跟踪内容 |
|------|------|-----|--------|----------|
| `lv_conf.h` | 489 | `LV_LOG_TRACE_MEM` | `1` | 内存分配/释放 |
| `lv_conf.h` | 490 | `LV_LOG_TRACE_TIMER` | `1` | 定时器操作 |
| `lv_conf.h` | 491 | `LV_LOG_TRACE_INDEV` | `1` | 输入设备读取 |
| `lv_conf.h` | 492 | `LV_LOG_TRACE_DISP_REFR` | `1` | 显示刷新周期 |
| `lv_conf.h` | 493 | `LV_LOG_TRACE_EVENT` | `1` | 事件分发 |
| `lv_conf.h` | 494 | `LV_LOG_TRACE_OBJ_CREATE` | `1` | 对象/控件创建 |
| `lv_conf.h` | 495 | `LV_LOG_TRACE_LAYOUT` | `1` | Flex/Grid 布局 |
| `lv_conf.h` | 496 | `LV_LOG_TRACE_ANIM` | `1` | 动画逻辑 |
| `lv_conf.h` | 497 | `LV_LOG_TRACE_CACHE` | `1` | 缓存操作 |

### 4.3 断言

| 文件 | 行号 | 宏 | 当前值 | 作用 |
|------|------|-----|--------|------|
| `lv_conf.h` | 506 | `LV_USE_ASSERT_NULL` | `1` | 检查 NULL 指针（很快，建议保持） |
| `lv_conf.h` | 507 | `LV_USE_ASSERT_MALLOC` | `1` | 检查内存分配成功（很快，建议保持） |
| `lv_conf.h` | 508 | `LV_USE_ASSERT_STYLE` | `0` | 检查样式初始化（较慢） |
| `lv_conf.h` | 509 | `LV_USE_ASSERT_MEM_INTEGRITY` | `0` | 检查堆完整性（很慢） |
| `lv_conf.h` | 510 | `LV_USE_ASSERT_OBJ` | `0` | 检查对象有效性（较慢） |
| `lv_conf.h` | 514 | `LV_ASSERT_HANDLER` | `while(1);` | 断言失败行为（当前：死循环停机） |
| `lv_conf.h` | 524 | `LV_USE_CHECK_ARG` | `1` | 校验函数参数 |

### 4.4 可视化调试叠加层

| 文件 | 行号 | 宏 | 当前值 | 作用 |
|------|------|-----|--------|------|
| `lv_conf.h` | 535 | `LV_USE_REFR_DEBUG` | `0` | 在刷新区域绘制彩色叠加层 |
| `lv_conf.h` | 538 | `LV_USE_LAYER_DEBUG` | `0` | 绘制图层边界 |
| `lv_conf.h` | 544 | `LV_USE_PARALLEL_DRAW_DEBUG` | `0` | 并行渲染可视化 |

---

## 5. 性能监控

| 文件 | 行号 | 宏 | 当前值 | 作用 |
|------|------|-----|--------|------|
| `lv_conf.h` | 1116 | `LV_USE_SYSMON` | `1` | 系统监视器组件 |
| `lv_conf.h` | 1130 | `LV_USE_PERF_MONITOR` | `1` | 在屏幕上显示 FPS + CPU 使用率 |
| `lv_conf.h` | 1132 | `LV_USE_PERF_MONITOR_POS` | `LV_ALIGN_BOTTOM_RIGHT` | 叠加层位置（右下角） |
| `lv_conf.h` | 1135 | `LV_USE_PERF_MONITOR_LOG_MODE` | `0` | 0=屏幕叠加；1=日志输出 |
| `lv_conf.h` | 1137 | `LV_USE_MEM_MONITOR` | `0` | 显示内存使用叠加层 |
| `lv_conf.h` | 1148 | `LV_USE_PROFILER` | `0` | 详细运行时性能分析器 |
| `lv_conf.h` | 1209 | `LV_USE_MONKEY` | `0` | 随机输入压力测试 |
| `lv_conf.h` | 1113 | `LV_USE_SNAPSHOT` | `0` | 屏幕截图 API |

**FPS 监视器使用说明：**
- 已默认开启（`LV_USE_SYSMON=1`，`LV_USE_PERF_MONITOR=1`）
- FPS 计数器显示在屏幕右下角
- 将 `LV_USE_PERF_MONITOR_LOG_MODE` 设为 `1` 可改为日志输出 FPS

---

## 6. 常用操作速查

### 开启调试日志
```
lv_conf.h L460:  #define LV_USE_LOG  1
lv_conf.h L469:  #define LV_LOG_LEVEL  LV_LOG_LEVEL_TRACE
```

### 切换到 CPU 刷新（关闭 DMA2D）
```
lv_conf_ext.h L28:  #define LV_USE_DMA2D_FLUSH  0
```

### 显示刷新区域（可视化调试）
```
lv_conf.h L535:  #define LV_USE_REFR_DEBUG  1
```

### 随机压力测试
```
lv_conf.h L1209:  #define LV_USE_MONKEY  1
```

### 调整 LVGL 堆大小
```
lv_conf.h L72:  #define LV_MEM_SIZE  (512 * 1024U)   // 512KB
```

### 把 LVGL 堆搬到 SDRAM
```
lv_conf.h L78:  #define LV_MEM_ADR  0xC0000000
```

### 修改目标帧率
```
lv_conf.h L91:  #define LV_DEF_REFR_PERIOD  10   // ~100 FPS
```

### 修改显示缓冲区大小
```
lv_port_disp.c L35:  #define BUF_LINES  30   // 60KB (省内存，帧率降低)
lv_port_disp.c L35:  #define BUF_LINES  100  // 200KB (提帧率，多用内存)
```