#ifndef VIDEOOUTPUT_H
#define VIDEOOUTPUT_H

#include "avframequeue.h"

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

class VideoOutput
{
public:
    VideoOutput(AVFrameQueue *frame_queue,int width,int height,double* audio_clock,
                AVRational video_time_base);
    ~VideoOutput();

    AVFrameQueue *frame_queue_;
    int video_width_;
    int video_height_;

    int init();
    SDL_Window * windows_;
    SDL_Renderer * renderer_;
    SDL_Texture * texture_;

    int MainLoop();
    void RefreshLoopWaitEvent(SDL_Event *event);
    void videoRefresh(double * remaining_time);
    
    SDL_Rect rect_;

    double video_clock_;
    double* audio_clock_;
    AVRational video_time_base_;
};

#endif // VIDEOOUTPUT_H
