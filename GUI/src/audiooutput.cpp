#include "audiooutput.h"

#include <QtGlobal>
#include <cstring>
#include <algorithm>

#include "log.h"

extern "C" {
#include "libavutil/channel_layout.h"
}

QtAudioDevice::QtAudioDevice(const AudioParams& srcParams,
                                                         const AudioParams& dstParams,
                                                         AVRational audioTimeBase,
                                                         AVFrameQueue* queue,
                                                         AudioClockState* clockState,
                                                         QObject* parent)
        : QIODevice(parent),
            src_tgt_(srcParams),
            dst_tgt_(dstParams),
            audio_time_base_(audioTimeBase),
            frame_queue_(queue),
            clock_state_(clockState) {
    open(QIODevice::ReadOnly);
}

QtAudioDevice::~QtAudioDevice() {
    stop();
}

void QtAudioDevice::start() {
    playing_ = true;
}

void QtAudioDevice::stop() {
    playing_ = false;
    if (swr_ctx_) {
        swr_free(&swr_ctx_);
        swr_ctx_ = nullptr;
    }
    if (audio_buf_) {
        av_free(audio_buf_);
        audio_buf_ = nullptr;
    }
    audio_buf_size_ = 0;
    audio_buf_index_ = 0;
}

qint64 QtAudioDevice::readData(char* data, qint64 maxlen) {
    if (!playing_ || !frame_queue_) {
        std::memset(data, 0, static_cast<size_t>(maxlen));
        return maxlen;
    }

    qint64 totalRead = 0;
    while (maxlen > 0) {
        if (audio_buf_index_ >= static_cast<int>(audio_buf_size_)) {
            audio_buf_index_ = 0;
            audio_buf_size_ = 0;

            AVFrame* frame = frame_queue_->pop(10);
            if (frame) {
                int64_t frameTs = frame->best_effort_timestamp;
                if (frameTs == AV_NOPTS_VALUE) {
                    frameTs = frame->pts;
                }
                if (clock_state_ && frameTs != AV_NOPTS_VALUE && audio_time_base_.num > 0 && audio_time_base_.den > 0) {
                    const double ptsSec = frameTs * av_q2d(audio_time_base_);
                    clock_state_->basePtsUs.store(static_cast<qint64>(ptsSec * 1000000.0));
                    clock_state_->writtenUs.store(0);
                }

                const int frameChannels = frame->ch_layout.nb_channels;
                const bool needResample =
                    (frame->format != dst_tgt_.fmt) ||
                    (frame->sample_rate != dst_tgt_.freq) ||
                    (frameChannels != dst_tgt_.channels);

                if (needResample && !swr_ctx_) {
                    AVChannelLayout outLayout;
                    av_channel_layout_default(&outLayout, dst_tgt_.channels);

                    int swrRet = swr_alloc_set_opts2(
                        &swr_ctx_,
                        &outLayout,
                        dst_tgt_.fmt,
                        dst_tgt_.freq,
                        &frame->ch_layout,
                        static_cast<AVSampleFormat>(frame->format),
                        frame->sample_rate,
                        0,
                        nullptr);
                    av_channel_layout_uninit(&outLayout);

                    if (swrRet < 0 || !swr_ctx_ || swr_init(swr_ctx_) < 0) {
                        LogError("QtAudioDevice swr init failed");
                        if (swr_ctx_) {
                            swr_free(&swr_ctx_);
                        }
                        av_frame_free(&frame);
                        continue;
                    }
                }

                if (swr_ctx_) {
                    const uint8_t** in = const_cast<const uint8_t**>(frame->extended_data);
                    int outSamples = frame->nb_samples * dst_tgt_.freq / frame->sample_rate + 256;
                    int outBytes = av_samples_get_buffer_size(nullptr, dst_tgt_.channels, outSamples, dst_tgt_.fmt, 0);
                    if (outBytes > 0) {
                        av_fast_malloc(&audio_buf_, &audio_buf_size_, static_cast<size_t>(outBytes));
                        int converted = swr_convert(swr_ctx_, &audio_buf_, outSamples, in, frame->nb_samples);
                        if (converted >= 0) {
                            audio_buf_size_ = static_cast<uint32_t>(
                                av_samples_get_buffer_size(nullptr, dst_tgt_.channels, converted, dst_tgt_.fmt, 1));
                        }
                    }
                } else {
                    int audioSize = av_samples_get_buffer_size(
                        nullptr,
                        frameChannels,
                        frame->nb_samples,
                        static_cast<AVSampleFormat>(frame->format),
                        1);
                    if (audioSize > 0) {
                        av_fast_malloc(&audio_buf_, &audio_buf_size_, static_cast<size_t>(audioSize));
                        std::memcpy(audio_buf_, frame->data[0], static_cast<size_t>(audioSize));
                        audio_buf_size_ = static_cast<uint32_t>(audioSize);
                    }
                }
                av_frame_free(&frame);
            } else {
                audio_buf_size_ = 512;
                av_fast_malloc(&audio_buf_, &audio_buf_size_, static_cast<size_t>(audio_buf_size_));
                std::memset(audio_buf_, 0, audio_buf_size_);
            }
        }

        int len1 = static_cast<int>(audio_buf_size_) - audio_buf_index_;
        if (len1 > maxlen) {
            len1 = static_cast<int>(maxlen);
        }

        if (audio_buf_) {
            std::memcpy(data + totalRead, audio_buf_ + audio_buf_index_, static_cast<size_t>(len1));
        } else {
            std::memset(data + totalRead, 0, static_cast<size_t>(len1));
        }

        const int bytesPerSec = dst_tgt_.freq * dst_tgt_.channels * av_get_bytes_per_sample(dst_tgt_.fmt);
        if (bytesPerSec > 0) {
            const qint64 deltaUs = static_cast<qint64>((1000000LL * len1) / bytesPerSec);
            if (clock_state_) {
                clock_state_->writtenUs.fetch_add(deltaUs);
                clock_state_->fallbackUs.fetch_add(deltaUs);
            }
        }

        maxlen -= len1;
        totalRead += len1;
        audio_buf_index_ += len1;
    }

    return totalRead;
}

qint64 QtAudioDevice::writeData(const char* data, qint64 len) {
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

AudioOutput::AudioOutput(const AudioParams& audio_params, AVRational audioTimeBase, AVFrameQueue* frame_queue, QObject* parent)
    : QObject(parent), src_tgt_(audio_params), audio_time_base_(audioTimeBase), frame_queue_(frame_queue) {
}

AudioOutput::~AudioOutput() {
    DeInit();
}

int AudioOutput::init() {
    QAudioFormat format;
    format.setSampleRate(src_tgt_.freq);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice info = QMediaDevices::defaultAudioOutput();
    if (!info.isFormatSupported(format)) {
        format = info.preferredFormat();
    }

    dst_tgt_.channels = format.channelCount();
    dst_tgt_.freq = format.sampleRate();
    dst_tgt_.frame_size = 1024;

    switch (format.sampleFormat()) {
    case QAudioFormat::Int16:
        dst_tgt_.fmt = AV_SAMPLE_FMT_S16;
        break;
    case QAudioFormat::Int32:
        dst_tgt_.fmt = AV_SAMPLE_FMT_S32;
        break;
    case QAudioFormat::Float:
        dst_tgt_.fmt = AV_SAMPLE_FMT_FLT;
        break;
    default:
        dst_tgt_.fmt = AV_SAMPLE_FMT_S16;
        break;
    }

    AVChannelLayout outLayout;
    av_channel_layout_default(&outLayout, dst_tgt_.channels);
    dst_tgt_.channel_layout = outLayout.u.mask;
    av_channel_layout_uninit(&outLayout);

    audioDevice_ = new QtAudioDevice(src_tgt_, dst_tgt_, audio_time_base_, frame_queue_, &clock_state_, this);
    audioSink_ = new QAudioSink(info, format, this);

    audioDevice_->start();
    audioSink_->setVolume(1.0f);
    audioSink_->start(audioDevice_);

    if (audioSink_->state() == QAudio::StoppedState && audioSink_->error() != QAudio::NoError) {
        LogError("AudioOutput::init failed, state:%d error:%d", static_cast<int>(audioSink_->state()), static_cast<int>(audioSink_->error()));
        return -1;
    }

    LogInfo("AudioOutput::init success via QAudioSink");
    return 0;
}

int AudioOutput::DeInit() {
    if (audioDevice_) {
        audioDevice_->stop();
    }
    if (audioSink_) {
        audioSink_->stop();
        delete audioSink_;
        audioSink_ = nullptr;
    }
    if (audioDevice_) {
        delete audioDevice_;
        audioDevice_ = nullptr;
    }
    return 0;
}

double* AudioOutput::getAudioClock() {
    audio_clock_ = currentClockSeconds();
    return &audio_clock_;
}

double AudioOutput::currentClockSeconds() const {
    const qint64 queuedUs = bufferedUs();
    const qint64 basePtsUs = clock_state_.basePtsUs.load();
    const qint64 writtenUs = clock_state_.writtenUs.load();
    const qint64 fallbackUs = clock_state_.fallbackUs.load();

    qint64 clockUs = 0;
    if (basePtsUs >= 0) {
        clockUs = basePtsUs + writtenUs - queuedUs;
    } else {
        clockUs = fallbackUs - queuedUs;
    }

    if (clockUs < 0) {
        clockUs = 0;
    }
    return static_cast<double>(clockUs) / 1000000.0;
}

void AudioOutput::resetClock() {
    clock_state_.basePtsUs.store(-1);
    clock_state_.writtenUs.store(0);
    clock_state_.fallbackUs.store(0);
    audio_clock_ = 0.0;
}

qint64 AudioOutput::bytesToUs(qint64 bytes) const {
    const int bytesPerSample = av_get_bytes_per_sample(dst_tgt_.fmt);
    const qint64 bytesPerSec = static_cast<qint64>(dst_tgt_.freq) * dst_tgt_.channels * bytesPerSample;
    if (bytesPerSec <= 0) {
        return 0;
    }
    return static_cast<qint64>((1000000LL * bytes) / bytesPerSec);
}

qint64 AudioOutput::bufferedUs() const {
    if (!audioSink_) {
        return 0;
    }
    const qint64 bufferBytes = audioSink_->bufferSize();
    const qint64 freeBytes = audioSink_->bytesFree();
    if (bufferBytes <= 0 || freeBytes < 0) {
        return 0;
    }

    const qint64 queuedBytes = std::clamp(bufferBytes - freeBytes, static_cast<qint64>(0), bufferBytes);
    return bytesToUs(queuedBytes);
}

void AudioOutput::play() {
    if (!audioSink_ || !audioDevice_) {
        return;
    }

    if (audioSink_->state() == QAudio::SuspendedState) {
        audioSink_->resume();
        return;
    }

    if (audioSink_->state() == QAudio::StoppedState && audioSink_->error() == QAudio::NoError) {
        audioDevice_->start();
        audioSink_->start(audioDevice_);
    }
}

void AudioOutput::pause() {
    if (!audioSink_) {
        return;
    }

    if (audioSink_->state() == QAudio::ActiveState || audioSink_->state() == QAudio::IdleState) {
        audioSink_->suspend();
    }
}

void AudioOutput::setVolume(qreal value) {
    volume_ = qBound(0.0, value, 1.0);
    if (!audioSink_) {
        return;
    }
    audioSink_->setVolume(static_cast<float>(muted_ ? 0.0 : volume_));
}

void AudioOutput::setMuted(bool muted) {
    muted_ = muted;
    if (!audioSink_) {
        return;
    }
    audioSink_->setVolume(static_cast<float>(muted_ ? 0.0 : volume_));
}
