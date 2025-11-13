#ifndef AVFRAMEQUEUE_H
#define AVFRAMEQUEUE_H

#include "queue.h"
#include "log.h"

#ifdef __cplusplus
extern "C"{
//#include "libavutil/avutil.h"
//#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}
#endif

class AVFrameQueue
{
public:
    AVFrameQueue();
    ~AVFrameQueue();

    void abort();
    int push(AVFrame *val);
    void release();
    int size();
    AVFrame *front();
    AVFrame *pop(const int timeout);
private:
    Queue<AVFrame *> AVF_queue_;
};

#endif // AVFRAMEQUEUE_H
