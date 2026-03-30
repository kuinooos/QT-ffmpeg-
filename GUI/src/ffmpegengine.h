#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>
#include <QImage>
#include <atomic>
#include <cmath>

// ---- FFmpeg 后端头文件 ----
#include "log.h"
#include "thread.h"
#include "queue.h"
#include "avpacketqueue.h"
#include "avframequeue.h"
#include "demuxthread.h"
#include "decodethread.h"
#include "audiooutput.h"

#ifdef __cplusplus
extern "C" {
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}
#endif

/**
 * FFmpegEngine: 整合层
 *
 * 管理整个 FFmpeg 解码管线（解复用 → 解码 → 音视频输出），
 * 并通过 Qt 信号向 PlayerBridge / QML 前端汇报状态。
 *
 * 视频输出：将 YUV 帧转换为 QImage(RGB32)，通过信号发送给 Qt 前端渲染。
 * 音频输出：使用 Qt QAudioSink 的 AudioOutput。
 */
class FFmpegEngine : public QObject {
    Q_OBJECT
public:
    explicit FFmpegEngine(QObject* parent = nullptr);
    ~FFmpegEngine();

    // --- 控制接口（由 PlayerBridge 调用） ---
    Q_INVOKABLE bool openFile(const QString& filePath);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void setVolume(qreal value);
    Q_INVOKABLE void setMuted(bool muted);

    // --- 状态查询 ---
    bool isPlaying() const { return playing_; }
    qint64 durationMs() const { return durationMs_; }
    qint64 positionMs() const { return positionMs_; }

signals:
    // 新的一帧视频已准备好，发送给前端渲染
    void frameReady(const QImage& frame);

    // 播放位置更新（毫秒）
    void positionChanged(qint64 positionMs);

    // 总时长更新（毫秒）
    void durationChanged(qint64 durationMs);

    // 播放状态变更：0=idle, 1=playing, 2=paused, 4=stopped
    void stateChanged(int state);

    // 发生错误
    void errorOccurred(const QString& message);

private:
    // 视频刷新定时器回调
    void onVideoRefreshTimer();
    double framePtsSeconds(const AVFrame* frame) const;
    void scheduleNextRefresh(int ms);

    // 清理所有资源
    void cleanup();

    // --- FFmpeg 管线组件 ---
    AVPacketQueue audioPacketQueue_;
    AVPacketQueue videoPacketQueue_;
    AVFrameQueue  audioFrameQueue_;
    AVFrameQueue  videoFrameQueue_;

    DemuxThread*  demuxThread_  = nullptr;
    DecodeThread* audioDecoder_ = nullptr;
    DecodeThread* videoDecoder_ = nullptr;
    AudioOutput*  audioOutput_  = nullptr;

    // --- 视频帧转换 (YUV → RGB) ---
    SwsContext*   swsCtx_   = nullptr;
    int videoWidth_  = 0;
    int videoHeight_ = 0;

    // --- 定时器驱动视频刷新 ---
    QTimer videoRefreshTimer_;

    // --- 状态变量 ---
    std::atomic<bool> playing_{false};
    qint64 durationMs_  = 0;
    qint64 positionMs_  = 0;

    // 音视频同步用
    AVRational audioTimeBase_{0, 0};
    AVRational videoTimeBase_{0, 0};
    double lastVideoPtsSec_ = 0.0;
    int droppedFrames_ = 0;

    static constexpr int kNominalRefreshMs = 33;
    static constexpr int kMinRefreshMs = 5;
    static constexpr int kMaxRefreshMs = 120;
    static constexpr int kMaxDropPerTick = 5;
    static constexpr double kSyncWindowSec = 0.04;
    static constexpr double kDropThresholdSec = -0.08;
    static constexpr double kMaxDelayCompensationSec = 0.08;
};
