# 音乐APP与GUI生成文件覆盖说明

## 1. 功能边界

音乐APP读取SD卡中的MP3文件，合并扫描`0:/MUSIC`和`0:/SYSTEM/MUSIC`。两个目录存在同名歌曲时优先保留`0:/MUSIC`版本，避免列表重复。当前最多保存512首歌曲，单个文件名缓冲为256字节，可覆盖FatFs当前255字节长文件名配置；播放时再按根目录编号拼出最长288字节路径。超过容量、文件名超限、重复项或目录读取异常会计入`skipped_count`，列表右上角以黄色`SKIP`显示，不再静默丢弃。

512条曲目表不放入内部ZI区。音乐服务初始化时通过`lv_malloc_zeroed()`从现有24MiB SDRAM LVGL内存池申请约128.5KiB，只保存根目录编号和原始文件名。状态中的`track_count`和`selected_index`使用16位类型，已由主机测试覆盖第512首和索引511。

播放器支持：

- 桌面第二页音乐图标进入APP；
- 曲目列表点击播放；
- 音乐APP使用独立`screen_music`屏幕，白色背景，不再作为桌面子容器常驻对象树；
- 独立屏幕顶部保留35像素导航栏、仅显示时分的时钟、状态图标和专用于下滑的空label；播放器区域严格采用参考工程实际启用的`LV_DEMO_MUSIC_LARGE=0`、`LV_DEMO_MUSIC_LANDSCAPE=0`单列Grid布局；
- 进入APP先显示原版97×97 Logo和镜像条纹启动动画，2秒后进入播放器过渡，标题、图标、控制、封面从3秒开始淡入；
- 播放器背景为纯白，不加载桌面壁纸，也不创建运行时缩放的波浪背景对象；
- 在播放器内纵向滑动或点击`ALL TRACKS`进入SD卡曲目页；
- 上一曲、播放/暂停、下一曲；
- 显示当前播放时间、估算总时长和只读播放进度条；
- 播放完成后自动进入下一曲；
- APP与导航栏音乐卡片同步曲名、播放状态和切歌操作；
- APP与导航栏音量滑块共享同一音量状态；
- 离开音乐APP或进入导航栏页面后继续播放。

当前解码链只支持MP3，不支持拖动进度条跳播，因此进度条明确设置为只读。已播放时间来自DMA实际完成的双声道样本量，不使用演示动画伪造播放时间。总时长在首帧得到码率后按“去除ID3v2后的音频字节数/码率”计算：CBR文件准确，VBR文件为估算值，避免为计算时长预扫整首文件而拖慢切歌。

曲目页面采用固定9行虚拟列表。无论SD卡中有几十首还是512首，LVGL只创建9组按钮、图标和文字对象，滚动时按可视索引复用，避免一次创建上千个对象导致卡顿。

环形频谱只在音乐APP屏幕存在且正在播放时按实际已播放时间更新；暂停时静止，页面删除后停止更新。频谱使用参考工程`spectrum_1`四频带数据，在250×250透明对象中按内半径77绘制20组左右镜像楔形条纹；LVGL 9用两个`lv_draw_triangle()`组成每个四边形，不再创建并动态缩放40个`lv_arc`。原版封面是无Alpha的`RGB565`方形资源，因此条纹必须在`LV_EVENT_DRAW_POST`阶段绘制，覆盖圆形图案外的白色方角；动态内半径负责避免条纹覆盖圆形封面内容。正常播放只失效250×250区域，Logo过渡阶段才临时扩展绘制范围。

频谱按33ms更新，但静态UI不跟随该节拍重复刷新。曲名、状态、播放图和列表选中态只在值变化时更新，进度文字和进度条每秒更新，避免相同label和图片每帧重复失效。

## 2. GUI生成文件修改

以下修改经过用户明确授权。所有新增位置均带`MUSIC APP USER PATCH`注释，GUI Guider重新生成后需要恢复。

### `lvgl_GUI/generated/gui_guider.h`

在桌面对象结构中增加：

- `DeskTop_img_musicAPP`
- `DeskTop_label_musicAPP`

并增加独立音乐屏幕及顶部栏字段：

- `screen_music`、`screen_music_del`
- `screen_music_cont_navigation_bar`
- `screen_music_label_navigation_bar_flag`（文本必须保持为空）
- `screen_music_digital_clock`及返回按钮、状态label
- `setup_scr_screen_music()`声明

### `lvgl_GUI/generated/gui_guider.c`

在`init_scr_del_flag()`中增加`screen_music_del = true`，使音乐屏幕按其他APP相同方式按需创建和自动删除。

### `lvgl_GUI/generated/setup_scr_screen_music.c`

新增1024×600白色独立屏幕。内容区从`y=35`开始由`music_player_ui_attach()`创建；音乐专用顶部栏同样使用纯白背景，返回、时间和状态图标使用深灰色，不加载或继承桌面壁纸。时钟格式为`时:分`，不显示秒。`screen_music_label_navigation_bar_flag`是空文本触控区，禁止删除。顶部下滑进入导航栏页面，导航栏向上返回时回到音乐APP而不是固定回到桌面。

### `lvgl_GUI/generated/setup_scr_DeskTop.c`

在桌面第二页第一行第5列增加80×80音乐图标，坐标为`x=450, y=35`；应用名称“音乐”位于`x=450, y=119`，使用16号字体，与同页其他APP名称保持一致。

桌面音乐图标使用GUI Guider生成的`_music_RGB565A8_80x80`，源文件为`lvgl_GUI/generated/images/_music_RGB565A8_80x80.c`，避免旧手工转换文件的RGB565颜色顺序问题。播放器封面及控件使用参考工程原版资源，由`Tools/convert_lvgl8_music_assets.ps1`机械转换到`lvgl_GUI/custom/music_demo_assets.c`。

### `lvgl_GUI/generated/events_init.c`

增加`DeskTop_img_musicAPP_event_handler()`，并在`events_init_DeskTop()`中注册。点击后调用`music_player_ui_open()`。

增加音乐屏幕返回、顶部下滑事件，并在导航栏返回事件中消费来源标志。所有新增位置均有`MUSIC APP USER PATCH`注释。

### `lvgl_GUI/generated/events_init.h`和`widgets_init.c`

声明`events_init_screen_music()`；RTC一秒定时回调在音乐屏幕存在时同步更新顶部时钟。

### `lvgl_GUI/generated/setup_scr_screen_navigation_bar.c`

导航栏音乐卡片的`screen_navigation_bar_label_9`改用`lv_font_myyh_20`。SD文件名可能包含大量中文，20号字库覆盖更完整，可减少方框缺字。

原有`DeskTop_label_navigation_bar_flag`空label和导航栏下滑事件未删除、未改名；音乐屏幕另有自己的同用途空label。

## 3. 导航栏音乐卡片映射

| 对象 | 功能 |
| --- | --- |
| `screen_navigation_bar_btn_10` | 进入音乐APP |
| `screen_navigation_bar_btn_11` | 上一曲 |
| `screen_navigation_bar_btn_12` | 下一曲 |
| `screen_navigation_bar_btn_14` | 播放或暂停 |
| `screen_navigation_bar_label_9` | 当前曲名，20号字体 |
| `screen_navigation_bar_slider_2` | 音量，0～100 |

导航页每次由GUI重新创建后，`music_player_ui_poll()`会重新绑定卡片事件并从统一音乐服务恢复显示状态。

## 4. 音量滑块修复

原实现收到每次`LV_EVENT_VALUE_CHANGED`后立即写ES8388左右声道寄存器。连续拖动会产生高密度阻塞I2C写，出现多滑几次后不再生效的现象。

现实现只缓存最新音量值，每次滑动事件都会把提交截止时间更新为当前时刻后50ms；拖动期间不写ES8388，停止变化50ms后只写一次最终值。APP和导航栏滑块均读取同一缓存状态，最终值不会被中间事件覆盖。桌面和导航音乐按钮只调用`music_player_ui_request_open()`置位，真正的SD扫描和切屏由下一次`music_player_ui_poll()`执行，禁止在LVGL按钮回调内清空当前对象树。串口`audio volume`命令仍可直接调用底层音量接口。

公共切屏函数还必须检查`lv_display_get_screen_loading()`：已有动画未完成时拒绝第二次切屏。不得在动画开始前调用`lv_obj_clean(active_screen)`，旧屏对象保留到LVGL发送`SCREEN_UNLOADED`后由`auto_del`整体删除；24MB LVGL SDRAM池允许动画期间短暂保留双屏。屏幕根对象删除时需将`gui_guider`中的对应根指针置NULL。RTC一秒回调更新任何时钟、日期、星期、闹钟或倒计时对象前，必须确认对象属于当前活动的对应根屏，不能只使用`lv_obj_is_valid()`判断旧指针。

## 5. 图标重新生成

在GUI Guider中使用`Docs/SD卡必备文件/PICTURE/music.png`生成80×80、`RGB565A8`图片，符号名保持`_music_RGB565A8_80x80`。生成后确认`setup_scr_DeskTop.c`引用该符号，并运行两个音乐契约。旧`lvgl_GUI/custom/music_icon_80.c`不再进入Keil工程。

参考工程原版界面资源需要重新转换时执行：

```powershell
& .\Tools\convert_lvgl8_music_assets.ps1 `
  -SourceDirectory '<参考工程>\Middlewares\LVGL\GUI_APP\demos\music\assets' `
  -OutputPath '.\lvgl_GUI\custom\music_demo_assets.c'
```

脚本只读取LVGL 8资源的16位非交换分支，将交错的RGB565+Alpha机械重排为LVGL 9要求的`RGB565A8`双平面格式，并校验每张图片的宽、高和数据长度。资源清单使用参考工程实际启用的272×42小波浪、97×97 Logo及原版小尺寸封面/控件；主页面按用户要求保持纯白，因此不创建波浪图片对象。`music_spectrum_1.h`是从同一参考工程机械复制的只读四频带数据。

## 6. GUI Guider重新生成后的恢复顺序

1. 在`gui_guider.h`恢复桌面音乐对象、`screen_music`全部字段及setup声明。
2. 在`gui_guider.c`恢复`screen_music_del = true`，并恢复`SCREEN LIFETIME USER PATCH`：切屏忙保护、禁止提前清空旧屏子对象以及屏幕DELETE时清空全局根指针。
3. 恢复独立`setup_scr_screen_music.c`并加入Keil `LVGL_GUI`组。
4. 在`setup_scr_DeskTop.c`恢复`_music_RGB565A8_80x80`声明、第二页图标和应用名称。
5. 在`events_init.c/.h`恢复音乐UI头文件、桌面入口、独立屏幕事件、导航来源返回逻辑及事件声明。桌面音乐图标必须调用`music_player_ui_request_open()`，不得在事件回调中直接调用`music_player_ui_open()`。`events_init.c`中的`LOCK RETURN USER PATCH`必须同时恢复：桌面、便签、信号处理、原神页和音乐页进入导航前记录来源；导航上划和锁屏解锁均调用`navigation_restore_saved_screen()`，首次开机默认来源为桌面。
6. 在`widgets_init.c`恢复音乐顶部时钟更新和`RTC OBJECT OWNERSHIP USER PATCH`；所有周期UI更新必须同时验证当前活动根屏和对象所有权。
7. 在`setup_scr_screen_navigation_bar.c`将`label_9`字体恢复为`lv_font_myyh_20`。
8. 运行`test_music_player_ui_contract`，确认桌面和音乐屏幕两个空label、独立白屏及下滑导航逻辑仍存在。
9. 确认GUI生成图片和`music_demo_assets.c`仍存在，并运行`test_music_player_keil_contract`检查生产源的Keil分组。

## 7. Keil工程分组

- `music_player_service.c`：`Application_Services`
- `music_player_ui.c`：`LVGL_GUI`
- `music_player_visual.c`：`LVGL_GUI`
- `setup_scr_screen_music.c`：`LVGL_GUI`
- `_music_RGB565A8_80x80.c`：`LVGL_IMG`
- `music_demo_assets.c`：`LVGL_IMG`

本轮未修改`.ioc`、`.uvoptx`或`.uvguix`。Keil编译必须在通知用户并得到授权后执行。
