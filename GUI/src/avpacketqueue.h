#ifndef AVPACKETQUEUE_H
#define AVPACKETQUEUE_H
#include "log.h"
#include "queue.h"

#ifdef __cplusplus
extern "C"{
//#include "libavutil/avutil.h"
//#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}
#endif

class AVPacketQueue
{
public:
    AVPacketQueue();
    ~AVPacketQueue();

    void abort();
    int push(AVPacket *val);
    void release();
    int size();
    AVPacket* front();
    AVPacket *pop(const int timeout);
private:
    Queue<AVPacket *> AVP_queue_;
};

#endif // AVPACKETQUEUE_H
