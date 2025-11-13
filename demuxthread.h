#ifndef DEMUXTHREAD_H
#define DEMUXTHREAD_H

#include "log.h"
#include <error.h>
#include "thread.h"
#include "avpacketqueue.h"

#ifdef __cplusplus
extern "C"{
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}
#endif

class DemuxThread : public Thread
{
public:
    DemuxThread(AVPacketQueue *audioQueue,AVPacketQueue *videoQueue);
    ~DemuxThread();
    int init(const char* url);
    int start();
    int stop();
    void run();
    static int s();

    AVCodecParameters *AudioCodecParameters();
    AVCodecParameters *VideoCodecParameters();
    AVRational VideoStreamTimebase();
private:
    char err2str[256] = {0};
    AVPacketQueue *m_audioQueue = NULL;
    AVPacketQueue *m_videoQueue = NULL;
    std::string url_;
    AVFormatContext *ifmt_ctx_ = NULL;
    int audio_index_ = -1;
    int video_index_ = -1;

};

#endif // DEMUXTHREAD_H
