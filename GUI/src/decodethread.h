#ifndef DECODETHREAD_H
#define DECODETHREAD_H

#include "log.h"
#include "thread.h"
#include "avpacketqueue.h"
#include "avframequeue.h"

#ifdef __cplusplus
extern "C"{
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}
#endif

class DecodeThread : public Thread
{
public:
    DecodeThread(AVPacketQueue* packet_queue,AVFrameQueue* frame_queue);
    ~DecodeThread();

    int init(AVCodecParameters *par);
    int start();
    int stop();

private:
    void run();
    char err2str[256] = {0};
    AVCodecContext *codec_ctx_ = NULL;//解码器上下文
    AVPacketQueue *packet_queue_;
    AVFrameQueue *frame_queue_;
};

#endif // DECODETHREAD_H
