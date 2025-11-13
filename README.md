# ffmpeg_player

一个基于 FFmpeg + SDL2（Qt 工程脚手架，仅使用 qmake，不使用 QtWidgets）的简易音视频播放器示例。实现了：
- 解复用（Demux）
- 音/视频独立解码线程
- 队列式缓冲（AVPacketQueue / AVFrameQueue）
- SDL 音频播放 + SDL 视频渲染
- 基于音频时钟的音视频同步（视频按音频时钟对齐）

作者邮箱：1832578485@qq.com

> 提醒：本仓库内包含 Windows 平台的 FFmpeg 头文件与 import library（.lib）以及 SDL2 头文件与 .lib；运行时仍需匹配版本的 DLL（FFmpeg 与 SDL2）放在可执行文件同目录或加入 PATH。


## 目录结构

```
ffmpeg_player/
  audiooutput.cpp/.h        # SDL 音频输出（重采样 + 回调拉流）
  videooutput.cpp/.h        # SDL 视频输出（YUV 绘制 + 同步）
  demuxthread.cpp/.h        # 解复用线程
  decodethread.cpp/.h       # 解码线程（音频/视频各一）
  avpacketqueue.cpp/.h      # 已解复用、待解码的包队列
  avframequeue.cpp/.h       # 已解码、待播放的帧队列
  queue.* / thread.*        # 简易队列/线程封装
  log.*                     # 简易日志
  main.cpp                  # 程序入口
  ffmpeg_player.pro         # qmake 工程配置
  SDL2-2.0.10/              # SDL2 头文件与库（Windows）
  ffmpeg-4.2.1-win32-dev/   # FFmpeg 头文件与 .lib（Windows）
```


## 架构与数据流

1) 解复用（DemuxThread）
- 读取输入媒体，按流类型分发到音频/视频包队列（AVPacketQueue）。

2) 解码（DecodeThread）
- 分别从对应的包队列取包，解码为 AVFrame，推入音频/视频帧队列（AVFrameQueue）。

3) 音频输出（AudioOutput）
- SDL 回调拉取音频帧；必要时通过 libswresample 重采样为设备参数（双声道、S16、freq 等）。
- 按实际送入音频设备的字节数推进音频时钟 audio_clock_（见下文“同步原理”）。

4) 视频输出（VideoOutput）
- 从视频帧队列取帧，将 YUV 更新到 SDL 纹理并渲染到窗口。
- 根据视频帧 PTS 与音频时钟的差值进行等待/渲染（简单同步策略）。


## 音视频同步原理（本工程实现）

本工程选用“音频主时钟，视频对齐音频”的策略：

- 音频时钟（Audio clock）
  - 在 SDL 音频回调 `fill_audio_pcm` 中，每成功拷贝 `len1` 字节到设备缓冲，就按输出格式推算已播放时长：
    - bytes_per_sec = freq × channels × bytes_per_sample
    - audio_clock_ += len1 / bytes_per_sec
  - 这意味着音频时钟近似于“已被设备消耗的时长”。

- 视频时钟（Video clock）
  - 每个视频帧取 PTS（单位秒）：`video_pts = frame->pts * av_q2d(video_time_base)`。
  - 计算与音频时钟差 `diff = video_pts - *audio_clock_`：
    - 若 diff > 0.05：视频超前，sleep(diff) 后再渲染。
    - 若 diff <= 0.05：到点或落后，立刻渲染。
  - 丢帧策略示例代码已留注释（当前未启用）。

> 说明：这是一个简化可用的同步实现。若需更严格的 A/V 对齐，可：
> - 使用 SDL_QueueAudio + SDL_GetQueuedAudioSize 获取“未播字节数”，精确计算已播放时长；
> - 为视频增加滞后阈值丢帧策略、以及播放速率微调（变速/重复帧）。


## 构建

本项目使用 qmake（Qt 工程），但未使用 Qt GUI，只是借助 qmake 组织工程与链接。建议使用 Qt Creator 打开 `ffmpeg_player.pro`：

- 平台：Windows
- 编译器/工具链：建议使用与 .lib 匹配的 MSVC Kit（因为本仓库提供的是 .lib 库）。
- 依赖：
  - FFmpeg 头文件与 .lib 已包含于 `ffmpeg-4.2.1-win32-dev/`（仅开发库，需自备共享运行库 DLL）。
  - SDL2 头文件与 .lib 已包含于 `SDL2-2.0.10/SDL2-2.0.10/`（运行时需 SDL2.dll）。

`ffmpeg_player.pro` 关键片段（已配置好包含/库）：

- INCLUDEPATH 指向 `ffmpeg-4.2.1-win32-dev/include` 和 `SDL2-2.0.10/.../include`
- LIBS 链接 FFmpeg 的 avformat/avcodec/avutil/... 以及 SDL2.lib

### 运行时依赖（重要）
- 将匹配版本的 FFmpeg 运行时 DLL（如 avcodec-58.dll、avformat-58.dll、avutil-56.dll 等）与 SDL2.dll 复制到可执行文件所在目录，或将其路径加入系统 PATH。
- 如果加载 DLL 失败，程序会在启动时就报错无法运行。


## 运行

当前 `main.cpp` 中使用了固定输入：
```cpp
ret = demux_thread.init("time.mp4");
```
建议：
- 将 `time.mp4` 放到可执行文件工作目录；或者
- 根据需要改为从命令行读取路径（Todo/可选优化）。

运行后：
- 窗口标题 `player`，按 ESC 退出；
- 控制台输出 FPS 和队列大小等日志；
- 会在工作目录生成 `dump.pcm`（调试导出的音频数据）。


## 常见问题（FAQ）

- 启动报找不到某某 DLL
  - 将 FFmpeg 与 SDL2 对应的 DLL 拷贝到可执行文件目录，或加入 PATH。

- 只有声音/只有画面
  - 检查 demux/decoding 是否正常（日志），检查解码出的帧是否被持续推入帧队列。

- 音视频不同步/抖动
  - 这是教学示例的简化同步。可按前述建议切换到 SDL_QueueAudio 精确字节法，或引入丢帧/追帧策略。


## 后续改进建议（Roadmap）
- [ ] main 支持命令行参数：输入文件路径
- [ ] 切换音频为 SDL_QueueAudio 模式，按队列未播字节精确计算音频时钟
- [ ] 视频落后丢帧/追帧策略，可配置阈值
- [ ] 支持拉流（RTSP/RTMP/HLS）与软硬解切换
- [ ] 跨平台构建（Linux/macOS 使用 pkg-config 配置 FFmpeg/SDL2）


## 许可与第三方依赖
- 本项目采用 MIT 许可证，详见仓库根目录的 `LICENSE` 文件。
- FFmpeg 遵循 LGPL/GPL 许可（具体取决于构建选项）；请遵照 FFmpeg 许可要求分发。
- SDL2 遵循 zlib 许可。

相关项目：
- FFmpeg: https://ffmpeg.org/
- SDL2: https://www.libsdl.org/


## 联系方式
- Email: 1832578485@qq.com

如在构建或运行中遇到问题，欢迎提 Issue 或邮件联系。
