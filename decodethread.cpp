#include "decodethread.h"

DecodeThread::DecodeThread(AVPacketQueue* pqueue,AVFrameQueue* fqueue)
    :   packet_queue_(pqueue) , frame_queue_(fqueue)
{

}

DecodeThread::~DecodeThread()
{
    if(thread_)   {
        stop();
    }
    if(codec_ctx_){
        avcodec_close(codec_ctx_);
    }
}

int DecodeThread::init(AVCodecParameters *par)
{
    if(!par){
        LogError("par is NULL!");
        return -1;
    }
    int ret;
    //分配并初始化一个新的 AVCodecContext（解码器上下文）。
    //参数可以传NULL，也可以传AVcodec
    codec_ctx_ = avcodec_alloc_context3(NULL);
    if(!codec_ctx_){
        LogError("avcodec_alloc_context3 failed!");
        return -1;
    }

    //AVCodecParameters 只保存了描述信息（比如编码器 ID、分辨率、采样率、像素格式等）。
    //但解码器需要一个完整的 AVCodecContext 来工作，必须把这些参数搬过去。
    ret = avcodec_parameters_to_context(codec_ctx_,par);
    if(ret < 0){
        av_strerror(ret,err2str,sizeof (err2str));//把 FFmpeg 的错误码（int 类型） 转换成人类能看懂的 错误信息字符串。
        LogError("avcodec_parameters_to_context failed, ret:%d,err2str:%s",ret,err2str);
        return -1;
    }

    //根据codec_id找到对应的解码器AVCodec
    AVCodec* codec = avcodec_find_decoder(codec_ctx_->codec_id);
    if(!codec){
        LogError("avcodec_find_decoder failed!");
        return -1;
    }

    //真正打开解码器，把 codec 和 codec_ctx_ 绑定。
    //把 codec 填到 codec_ctx->codec 字段里。
    ret = avcodec_open2(codec_ctx_,codec,NULL);
    if(ret < 0){
        av_strerror(ret,err2str,sizeof (err2str));//把 FFmpeg 的错误码（int 类型） 转换成人类能看懂的 错误信息字符串。
        LogError("avcodec_open2 failed, ret:%d,err2str:%s",ret,err2str);
        return -1;
    }

    LogInfo("DecodeThread::init finished!");
    return 0;
}

int DecodeThread::start()
{
    thread_ = new std::thread(&DecodeThread::run,this);
    if(!thread_){
        LogError("DecodeThread.start() failed");
        return -1;
    }
    return 0;
}

void DecodeThread::run()
{
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    int ret;
    while (abort_ != 1){
        /*
         * decodethread 在 frame 队列超过 10 时暂停
         * ，给渲染/播放线程时间消费，避免解码过深导致渲染延迟上升。
         */
        if(frame_queue_->size() > 10){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;

        }

        pkt = packet_queue_->pop(10);

        if(pkt == nullptr){
            LogInfo("not get packet_queue pkt");
            continue;
        }
        ret = avcodec_send_packet(codec_ctx_,pkt);
        if(ret < 0){
            av_strerror(ret,err2str,sizeof (err2str));//把 FFmpeg 的错误码（int 类型） 转换成人类能看懂的 错误信息字符串。
            LogError("avcodec_send_packet failed, ret:%d,err2str:%s",ret,err2str);
            break;
        }
        while(true){
            //从解码器接收解码后的帧（对于解码操作）
            ret = avcodec_receive_frame(codec_ctx_,frame);
            if(ret == 0){
                frame_queue_->push(frame);
                LogInfo("DecodeThread::run() %s frame_queue size : %d",codec_ctx_->codec->name,frame_queue_->size());
            }else if(ret == AVERROR(EAGAIN)){
                break;
            }else{
                abort_ = 1;
                av_strerror(ret,err2str,sizeof (err2str));//把 FFmpeg 的错误码（int 类型） 转换成人类能看懂的 错误信息字符串。
                LogError("avcodec_receive_frame failed, ret:%d,err2str:%s",ret,err2str);
                break;
            }
            av_frame_unref(frame);
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
    av_frame_free(&frame);

    LogInfo("DecodeThread run finished!");
}

int DecodeThread::stop()
{
    Thread::stop();//结束线程thread_
    return 0;
}
