#include "avframequeue.h"

AVFrameQueue::AVFrameQueue()
{


}

AVFrameQueue::~AVFrameQueue()
{

}

void AVFrameQueue::abort()
{
    release();
    AVF_queue_.abort();//终止队列行为
}

int AVFrameQueue::push(AVFrame *val)
{
    AVFrame* tmp_frame = av_frame_alloc();
    av_frame_move_ref(tmp_frame, val);
    return AVF_queue_.push(tmp_frame);//normal : 0 , abort_ : -1
}

void AVFrameQueue::release()
{
    AVFrame* avf;
    int ret = 0;
    while(true){
         ret = AVF_queue_.pop(avf,1);
         if(ret < 0){
             break;
         }

         av_frame_unref(avf);
    }
    av_frame_free(&avf);
}

int AVFrameQueue::size()
{
    return AVF_queue_.size();
}

AVFrame *AVFrameQueue::front()
{
    AVFrame *tmp_frame = NULL;
    int ret = AVF_queue_.front(tmp_frame);
    if(ret < 0) {
        if(ret == -1)
            LogError("AVFrameQueue::Pop failed");
    }
    return tmp_frame;
}

AVFrame *AVFrameQueue::pop(const int timeout)
{
    AVFrame* avf;
    int ret = AVF_queue_.pop(avf,timeout);
    if(ret == -1){
        LogError("Abort, pop failed!");
    }

    return avf;
}
