#pragma once

#include <QObject>
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>
#include <QtGlobal>
#include <atomic>
#include <cstdint>

#include "avframequeue.h"

#ifdef __cplusplus
extern "C" {
#include "libswresample/swresample.h"
#include "libavutil/samplefmt.h"
}
#endif

typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
} AudioParams;

struct AudioClockState {
    std::atomic<qint64> basePtsUs{-1};
    std::atomic<qint64> writtenUs{0};
    std::atomic<qint64> fallbackUs{0};
};

class QtAudioDevice : public QIODevice {
    Q_OBJECT
public:
    QtAudioDevice(const AudioParams& srcParams,
                  const AudioParams& dstParams,
                  AVRational audioTimeBase,
                  AVFrameQueue* queue,
                  AudioClockState* clockState,
                  QObject* parent = nullptr);
    ~QtAudioDevice() override;

    void start();
    void stop();

protected:
    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return 4096 + QIODevice::bytesAvailable(); }
    qint64 readData(char* data, qint64 maxlen) override;
    qint64 writeData(const char* data, qint64 len) override;

private:
    AudioParams src_tgt_;
    AudioParams dst_tgt_;
    AVRational audio_time_base_{0, 0};
    AVFrameQueue* frame_queue_ = nullptr;
    AudioClockState* clock_state_ = nullptr;

    SwrContext* swr_ctx_ = nullptr;
    uint8_t* audio_buf_ = nullptr;
    uint32_t audio_buf_size_ = 0;
    int audio_buf_index_ = 0;

    std::atomic<bool> playing_{false};
};

class AudioOutput : public QObject {
    Q_OBJECT
public:
    AudioOutput(const AudioParams& audio_params, AVRational audioTimeBase, AVFrameQueue* frame_queue, QObject* parent = nullptr);
    ~AudioOutput() override;

    int init();
    int DeInit();
    void play();
    void pause();
    void setVolume(qreal value);
    void setMuted(bool muted);

    double* getAudioClock();
    double currentClockSeconds() const;
    void resetClock();

private:
    qint64 bytesToUs(qint64 bytes) const;
    qint64 bufferedUs() const;

    AudioParams src_tgt_;
    AudioParams dst_tgt_;
    AVRational audio_time_base_{0, 0};
    AVFrameQueue* frame_queue_ = nullptr;

    QAudioSink* audioSink_ = nullptr;
    QtAudioDevice* audioDevice_ = nullptr;
    double audio_clock_ = 0.0;
    AudioClockState clock_state_;
    qreal volume_ = 1.0;
    bool muted_ = false;
};
