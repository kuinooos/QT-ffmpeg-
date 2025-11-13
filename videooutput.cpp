#include "videooutput.h"

#include "log.h"
#include <thread>


VideoOutput::VideoOutput(AVFrameQueue *frame_queue, int width, int height, double* audio_clock,AVRational video_time_base)
    :frame_queue_(frame_queue),video_width_(width),video_height_(height),audio_clock_(audio_clock),video_time_base_(video_time_base)
{

}

VideoOutput::~VideoOutput()
{
    if (texture_) {
           SDL_DestroyTexture(texture_);
           texture_ = nullptr;
       }
       if (renderer_) {
           SDL_DestroyRenderer(renderer_);
           renderer_ = nullptr;
       }
       if (windows_) {
           SDL_DestroyWindow(windows_);
           windows_ = nullptr;
       }
       SDL_Quit(); // 注意：如果其他模块也用了SDL，可能需要全局管理SDL_Quit()

}

int VideoOutput::init()
{
    if(SDL_Init(SDL_INIT_VIDEO))   {
        LogError("SDL_Init(SDL_INIT_VIDEO) failed");
        return -1;
    }

    windows_ = SDL_CreateWindow("player",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
                     video_width_,video_height_,SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if(!windows_){
        LogError("SDL_CreateWindow failed");
        goto failed;
    }

    //在刚刚创建的窗口上建立一个“渲染器”（Renderer），它是绘制图像的关键对象。
    renderer_ = SDL_CreateRenderer(windows_,-1,0);
    if(!renderer_){
        LogError("SDL_CreateRenderer failed");
        goto failed;
    }

    //创建一块 纹理缓冲区，用于存储视频帧数据。
    texture_ = SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STREAMING,video_width_,video_height_);
    if(!texture_){
        LogError("SDL_CreateTexture failed");
        goto failed;
    }

    LogInfo("VideoOutput::init() finished");
    return 0;

failed:
    if (texture_) {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (windows_) {
        SDL_DestroyWindow(windows_);
        windows_ = nullptr;
    }
    SDL_Quit();
    return -1;
}

int VideoOutput::MainLoop()
{
    LogInfo("VideoOutput::MainLoop()");
    int sign = 0 ;
    SDL_Event event;
    while(true){
        LogInfo("sign %d",sign);
        //读取事件
        RefreshLoopWaitEvent(&event);
        sign++;
        switch (event.type) {
            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_ESCAPE){
                    LogInfo("esc key down");
                    return 0;
                }
            break;
            case SDL_QUIT:
                LogInfo("SQL_QUIT");
                return 0;
            break;
        default:
            break;
        }
    }
}

#define REFRESH_RATE 0.04
void VideoOutput::RefreshLoopWaitEvent(SDL_Event *event)
{
    double remaining_time = 0.0;
    SDL_PumpEvents();
    while(!SDL_PeepEvents(event,1,SDL_GETEVENT,SDL_FIRSTEVENT,SDL_LASTEVENT)){
        if(remaining_time > 0.0){
            std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(remaining_time * 1000.0)));
        }
        remaining_time = REFRESH_RATE;
        //尝试刷新画面
        videoRefresh(&remaining_time);
        SDL_PumpEvents();
    }
}

void VideoOutput::videoRefresh(double *remaining_time)
{
    AVFrame* frame = nullptr;
    frame = frame_queue_->front();
    if(frame){
       // 1. 获取视频帧时间戳（单位：秒）
       //video_stream_->time_base
       double pts = frame->pts * av_q2d(video_time_base_);
       video_clock_ = pts;

       // 2. 计算与音频时钟的差
       double diff = video_clock_ - *audio_clock_;

       // 3. 同步逻辑
       if (diff > 0.05) {
           // 视频快于音频，稍微等待
           std::this_thread::sleep_for(std::chrono::milliseconds((int)(diff * 1000)));
       }/* else if (diff < -0.05) {
           // 视频太慢，直接丢帧
           frame_queue_->pop(10);
           av_frame_unref(frame);
           return;
       }*/

        rect_.x = 0;
        rect_.y = 0;
        rect_.h = video_height_;
        rect_.w = video_width_;
        SDL_UpdateYUVTexture(texture_,
                             &rect_,
                             frame->data[0],frame->linesize[0],
                             frame->data[1],frame->linesize[1],
                             frame->data[2],frame->linesize[2]
                );
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_,texture_,NULL,&rect_);
    SDL_RenderPresent(renderer_);

    // ✅ 播放计数
    static int frame_count = 0;
    static auto last_time = std::chrono::steady_clock::now();
    frame_count++;

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - last_time).count();

    // 每隔 1 秒打印一次 FPS
    if (elapsed >= 1.0) {
        double fps = frame_count / elapsed;
        LogInfo("FPS: %.2f (queue size=%d)", fps, frame_queue_->size());
        frame_count = 0;
        last_time = now;
    }

    frame = frame_queue_->pop(10);
    av_frame_free(&frame);
    }else{
        LogInfo("VideoOutput::videoRefresh :No frame in queue");
        return;
    }
}


