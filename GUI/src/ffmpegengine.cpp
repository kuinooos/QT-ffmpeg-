#include "ffmpegengine.h"

#include <QFileInfo>
#include <QUrl>
#include <QDebug>

extern "C" {
#include "libavutil/channel_layout.h"
}

FFmpegEngine::FFmpegEngine(QObject* parent)
    : QObject(parent)
{
    LogInit();

    // 单次触发，便于每帧动态调整刷新间隔。
    videoRefreshTimer_.setSingleShot(true);
    videoRefreshTimer_.setInterval(kNominalRefreshMs);
    connect(&videoRefreshTimer_, &QTimer::timeout, this, &FFmpegEngine::onVideoRefreshTimer);
}

FFmpegEngine::~FFmpegEngine()
{
    cleanup();
}

bool FFmpegEngine::openFile(const QString& filePath)
{
    // 先清理上一次的资源
    cleanup();

    // 处理 QML FileDialog 返回的 file:// URL
    QString localPath = filePath;
    if (localPath.startsWith("file:///")) {
        localPath = QUrl(localPath).toLocalFile();
    }

    QFileInfo fi(localPath);
    if (!fi.exists()) {
        emit errorOccurred("File not found: " + localPath);
        return false;
    }

    qDebug() << "[FFmpegEngine] Opening:" << localPath;

    // ---- 1. 解复用线程 ----
    demuxThread_ = new DemuxThread(&audioPacketQueue_, &videoPacketQueue_);
    int ret = demuxThread_->init(localPath.toUtf8().constData());
    if (ret != 0) {
        emit errorOccurred("DemuxThread init failed");
        cleanup();
        return false;
    }

    ret = demuxThread_->start();
    if (ret != 0) {
        emit errorOccurred("DemuxThread start failed");
        cleanup();
        return false;
    }

    // 获取视频宽高
    AVCodecParameters* videoPar = demuxThread_->VideoCodecParameters();
    AVCodecParameters* audioPar = demuxThread_->AudioCodecParameters();
    if (!videoPar || !audioPar) {
        emit errorOccurred("No audio or video stream found");
        cleanup();
        return false;
    }

    videoWidth_  = videoPar->width;
    videoHeight_ = videoPar->height;
    audioTimeBase_ = demuxThread_->AudioStreamTimebase();
    videoTimeBase_ = demuxThread_->VideoStreamTimebase();

    // 获取总时长（秒 → 毫秒）
    // 注意：demuxThread 内部的 ifmt_ctx_ 可以拿到 duration
    // 这里暂时设为 0，后续可通过扩展 DemuxThread 获取
    durationMs_ = 0; // TODO: 从 DemuxThread 获取 ifmt_ctx_->duration
    emit durationChanged(durationMs_);

    // ---- 2. 音频解码线程 ----
    audioDecoder_ = new DecodeThread(&audioPacketQueue_, &audioFrameQueue_);
    ret = audioDecoder_->init(audioPar);
    if (ret != 0) {
        emit errorOccurred("Audio decoder init failed");
        cleanup();
        return false;
    }
    audioDecoder_->start();

    // ---- 3. 视频解码线程 ----
    videoDecoder_ = new DecodeThread(&videoPacketQueue_, &videoFrameQueue_);
    ret = videoDecoder_->init(videoPar);
    if (ret != 0) {
        emit errorOccurred("Video decoder init failed");
        cleanup();
        return false;
    }
    videoDecoder_->start();

    // ---- 4. 音频输出 (Qt QAudioSink) ----
    AudioParams audioParams = {0};
    audioParams.channels       = audioPar->ch_layout.nb_channels;
    if (audioPar->ch_layout.u.mask != 0) {
        audioParams.channel_layout = audioPar->ch_layout.u.mask;
    } else {
        AVChannelLayout fallback;
        av_channel_layout_default(&fallback, audioParams.channels);
        audioParams.channel_layout = fallback.u.mask;
        av_channel_layout_uninit(&fallback);
    }
    audioParams.fmt            = (enum AVSampleFormat)audioPar->format;
    audioParams.freq           = audioPar->sample_rate;
    audioParams.frame_size     = audioPar->frame_size;

    audioOutput_ = new AudioOutput(audioParams, audioTimeBase_, &audioFrameQueue_);
    ret = audioOutput_->init();
    if (ret != 0) {
        emit errorOccurred("AudioOutput init failed");
        cleanup();
        return false;
    }

    // ---- 5. 初始化 SwsContext (YUV→RGB32) ----
    swsCtx_ = sws_getContext(
        videoWidth_, videoHeight_, AV_PIX_FMT_YUV420P,
        videoWidth_, videoHeight_, AV_PIX_FMT_RGB32,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
    if (!swsCtx_) {
        emit errorOccurred("sws_getContext failed");
        cleanup();
        return false;
    }

    qDebug() << "[FFmpegEngine] Open success, video:" << videoWidth_ << "x" << videoHeight_;
    droppedFrames_ = 0;
    lastVideoPtsSec_ = 0.0;
    emit stateChanged(0); // Idle
    return true;
}

void FFmpegEngine::play()
{
    if (!demuxThread_) {
        emit errorOccurred("No media opened");
        return;
    }

    if (audioOutput_) {
        audioOutput_->resetClock();
        audioOutput_->play();
    }

    playing_ = true;
    scheduleNextRefresh(kNominalRefreshMs);
    emit stateChanged(1); // Playing
}

void FFmpegEngine::pause()
{
    if (audioOutput_) {
        audioOutput_->pause();
    }

    playing_ = false;
    videoRefreshTimer_.stop();
    emit stateChanged(2); // Paused
}

void FFmpegEngine::stop()
{
    playing_ = false;
    videoRefreshTimer_.stop();
    positionMs_ = 0;
    emit positionChanged(0);
    emit stateChanged(4); // Stopped
    cleanup();
}

void FFmpegEngine::setVolume(qreal value)
{
    if (!audioOutput_) {
        return;
    }
    audioOutput_->setVolume(value);
}

void FFmpegEngine::setMuted(bool muted)
{
    if (!audioOutput_) {
        return;
    }
    audioOutput_->setMuted(muted);
}

// ---- 视频刷新：从 videoFrameQueue 取帧 → 转 QImage → 发信号 ----
void FFmpegEngine::onVideoRefreshTimer()
{
    if (!playing_ || !swsCtx_) {
        return;
    }

    AVFrame* frame = videoFrameQueue_.front();
    if (!frame) {
        scheduleNextRefresh(kMinRefreshMs);
        return;
    }

    double audioClock = NAN;
    bool audioClockValid = false;
    if (audioOutput_) {
        audioClock = audioOutput_->currentClockSeconds();
        audioClockValid = std::isfinite(audioClock);
    }

    // 视频慢时连续丢弃过期帧，直到回到同步窗口或达到本轮丢帧上限。
    int droppedNow = 0;
    double videoPts = framePtsSeconds(frame);
    if (audioClockValid) {
        while (frame && droppedNow < kMaxDropPerTick) {
            const double diff = videoPts - audioClock;
            if (diff >= kDropThresholdSec || videoFrameQueue_.size() <= 1) {
                break;
            }

            AVFrame* outdated = videoFrameQueue_.pop(1);
            if (outdated) {
                av_frame_free(&outdated);
                ++droppedFrames_;
                ++droppedNow;
            }

            frame = videoFrameQueue_.front();
            if (!frame) {
                break;
            }
            videoPts = framePtsSeconds(frame);
        }
    }

    if (!frame) {
        scheduleNextRefresh(kMinRefreshMs);
        return;
    }

    videoPts = framePtsSeconds(frame);
    if (!std::isfinite(videoPts)) {
        videoPts = static_cast<double>(positionMs_) / 1000.0;
    }

    // --- YUV → RGB32 转换 ---
    QImage img(videoWidth_, videoHeight_, QImage::Format_RGB32);
    uint8_t* dstData[1]     = { img.bits() };
    int      dstLinesize[1] = { static_cast<int>(img.bytesPerLine()) };

    sws_scale(swsCtx_,
              frame->data, frame->linesize,
              0, videoHeight_,
              dstData, dstLinesize);

    emit frameReady(img);

    // --- 更新播放位置 ---
    lastVideoPtsSec_ = videoPts;
    positionMs_ = static_cast<qint64>(videoPts * 1000.0);
    emit positionChanged(positionMs_);

    // --- 消费这一帧 ---
    AVFrame* popped = videoFrameQueue_.pop(10);
    if (popped) {
        av_frame_free(&popped);
    }

    int nextRefreshMs = kNominalRefreshMs;
    if (audioClockValid) {
        const double diff = videoPts - audioClock;
        if (diff > kSyncWindowSec) {
            const double waitSec = 0.033 + std::min(diff, kMaxDelayCompensationSec);
            nextRefreshMs = static_cast<int>(waitSec * 1000.0);
        } else if (diff < -kSyncWindowSec) {
            nextRefreshMs = kMinRefreshMs;
        }
    }
    scheduleNextRefresh(nextRefreshMs);
}

double FFmpegEngine::framePtsSeconds(const AVFrame* frame) const
{
    if (!frame) {
        return NAN;
    }

    int64_t ts = frame->best_effort_timestamp;
    if (ts == AV_NOPTS_VALUE) {
        ts = frame->pts;
    }

    if (ts != AV_NOPTS_VALUE && videoTimeBase_.num > 0 && videoTimeBase_.den > 0) {
        return ts * av_q2d(videoTimeBase_);
    }
    return NAN;
}

void FFmpegEngine::scheduleNextRefresh(int ms)
{
    if (!playing_) {
        return;
    }

    const int clamped = qBound(kMinRefreshMs, ms, kMaxRefreshMs);
    videoRefreshTimer_.start(clamped);
}

void FFmpegEngine::cleanup()
{
    playing_ = false;
    videoRefreshTimer_.stop();

    if (demuxThread_) {
        demuxThread_->stop();
        delete demuxThread_;
        demuxThread_ = nullptr;
    }
    if (audioDecoder_) {
        audioDecoder_->stop();
        delete audioDecoder_;
        audioDecoder_ = nullptr;
    }
    if (videoDecoder_) {
        videoDecoder_->stop();
        delete videoDecoder_;
        videoDecoder_ = nullptr;
    }
    if (audioOutput_) {
        audioOutput_->DeInit();
        delete audioOutput_;
        audioOutput_ = nullptr;
    }
    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
    }

    positionMs_ = 0;
    durationMs_ = 0;
    videoWidth_  = 0;
    videoHeight_ = 0;
    audioTimeBase_ = AVRational{0, 0};
    videoTimeBase_ = AVRational{0, 0};
    lastVideoPtsSec_ = 0.0;
    droppedFrames_ = 0;
}
