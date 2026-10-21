# AVI视频音频播放与生成文件覆盖说明

## 1. 功能范围

当前AVI播放器在原MJPEG画面播放基础上增加音频解析与播放，覆盖两条入口：

- SD文件浏览器点击`.avi`后进入独立视频页面；
- 桌面“原神”图标触发的全屏直接播放路径。

播放视频前会先停止后台音乐，避免音乐与视频同时占用SAI1和ES8388。视频结束、用户返回、文件读取失败或解码失败时统一停止视频音频流并释放SAI资源。

当前支持的AVI音频格式：

- `0x0001`：16位PCM，单声道自动复制为双声道，双声道原样输出；
- `0x0055`：MP3，使用工程现有Helix解码器进行跨AVI块流式解码。

当前不支持8位PCM、MP2、AC3及其他压缩格式。不支持的音频格式会打印`[AVI-AUDIO] unsupported`，视频画面继续播放，不会把压缩数据直接送入SAI产生噪声。

## 2. AVI解析补充

`Core/Middlewares/MJPEG/avi.h/.c`新增以下音频字段：

- `AudioByteRate`：平均字节率；
- `AudioBlockAlign`：PCM块对齐或压缩格式块信息；
- `BitsPerSample`：PCM位宽；
- `AVI_AUDIO_FORMAT_PCM`和`AVI_AUDIO_FORMAT_MP3`格式常量。

`avi_parse_audio_format()`按小端格式有界解析`strf`块，并拒绝截断数据。`avi_init()`的音频在前和视频在前两种流顺序均复用该接口。

当首个音频数据块不在16KB头缓冲范围内时，`avi_init()`可能返回`AVI_STREAM_ERR`，但已解析的音频格式元数据仍保留。播放器不再因此清空音频信息，而是在进入`movi`后按实际`00wb/01wb`块播放。

## 3. 音频流解码

新增文件：

- `Core/Peripheral/Inc/avi_audio_decoder.h`
- `Core/Peripheral/Src/avi_audio_decoder.c`

解码器使用16KB有界输入缓冲，缓冲通过`.audio_dma`段固定放在AXI SRAM。PCM按完整采样帧消费，奇数长度RIFF块的填充字节不会送入解码器。MP3数据允许跨多个AVI音频块累计，达到安全输入量后再调用Helix，避免把普通分块边界误判为格式错误。

MP3解码输出统一为16位双声道。单声道MP3在同一输出缓冲中从后向前复制，不新增2304样本临时PCM大缓冲。

## 4. SAI与DMA行为

`audio_playback_service`新增视频流模式：

- `audio_stream_start()`：申请SAI、准备ES8388、按AVI采样率配置SAI并打开流解码器；
- `audio_stream_push()`：提交一个AVI音频块；
- `audio_stream_finish()`：标记输入结束；
- `audio_stream_stop()`：停止DMA、关闭解码器并释放SAI。

音乐文件模式继续使用原4608样本半缓冲；AVI流模式使用2304样本半缓冲，以容纳一个Helix最大输出帧。两种模式复用现有`g_audio_pcm` AXI DMA缓冲和同一组HAL SAI回调，不创建第二套DMA中断。

视频音频数据暂时不足时，对当前DMA半区补静音并增加`underrun_count`，禁止读取未初始化数据或越界。AVI头采样率与MP3实际帧采样率不一致时停止音频流并报告错误。

## 5. GUI生成文件修改

`lvgl_GUI/generated/events_init.c`属于GUI Guider生成文件。本次因AVI打开、定时器和全屏直接播放逻辑均位于该文件，用户已授权修改。新增位置使用`AVI AUDIO USER PATCH`标记。

主要修改：

- 引入`audio_playback_service.h`和`music_player_service.h`；
- 两条AVI入口开始前调用`music_player_stop()`；
- 根据`g_avix.AudioType/SampleRate/Channels/BitsPerSample`启动音频；
- `00wb/01wb`块由原来的`f_lseek()`跳过改为读取并提交解码；
- 全屏阻塞播放循环显式调用`audio_poll()`；
- 所有停止路径调用`avi_player_audio_stop()`。

GUI Guider重新生成后，需要搜索`AVI AUDIO USER PATCH`并恢复以上修改。恢复后运行：

```powershell
gcc -std=c99 -Wall -Wextra -Werror Tests/host/test_avi_audio_integration_contract.c -o Tests/host/test_avi_audio_integration_contract.exe
& Tests/host/test_avi_audio_integration_contract.exe
```

## 6. Keil工程与验证

`MDK-ARM/H743_LCD_Keil5.uvprojx`的`Peripheral_Drivers`组新增唯一条目`avi_audio_decoder.c`。未修改`.uvoptx`或`.uvguix`。

软件侧测试包括：

- `test_avi_audio_format`
- `test_avi_audio_decoder`
- `test_audio_playback_service`
- `test_avi_audio_integration_contract`
- `test_avi_audio_keil_contract`
- `test_mp3_decoder_service`
- `test_music_player_service`

Keil构建后必须检查：

- 新增错误和新增警告均为0；
- `g_audio_pcm`与`g_avi_audio_input`均位于`0x24000000~0x2407FFFF`；
- 两个缓冲与相机`g_camera_chunk_buffers`不重叠；
- AXI SRAM剩余空间仍为正；
- `.lnp`继续使用`H743_LCD_Keil5_custom.sct`。

实机优先验证参考资源中的两种格式：11025Hz双声道16位PCM AVI和44100Hz双声道MP3 AVI。重点观察声音速度、左右声道、画面流畅度、音画起始偏差、`underrun_count`以及退出后音乐能否重新播放。
