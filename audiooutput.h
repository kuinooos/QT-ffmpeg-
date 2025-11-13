#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#ifdef __cplusplus  ///
extern "C"
{
// 包含ffmpeg头文件
//#include "libavutil/avutil.h"
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "libswresample/swresample.h"
}
#endif

#include "avframequeue.h"

typedef struct AudioParams
{
    int freq;// 采样率 (sample rate)
    int channels;// 通道数
    int64_t channel_layout;// 声道布局
    enum AVSampleFormat fmt;// 采样格式 (sample format)
    int frame_size;// 每帧大小
} AudioParams;

class AudioOutput
{
public:
    AudioOutput(const AudioParams &audio_params,AVFrameQueue *frame_queue);
    ~AudioOutput();
    int init();
    int DeInit();

    AudioParams src_tgt_;//解码后的参数
    AudioParams dst_tgt_;//SDL实际输出的格式

    AVFrameQueue *frame_queue_ = NULL;
    struct SwrContext *swr_ctx_ = NULL;

    int32_t audio_buf_size;
    int32_t audio_buf_index;
    uint8_t *audio_buf1_;
    uint32_t audio_buf1_size;
    uint8_t *audio_buf_;
    double audio_clock_ = 0.0;

    double* getAudioClock();
};

#endif // AUDIOOUTPUT_H
