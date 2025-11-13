#include "demuxthread.h"

DemuxThread::DemuxThread(AVPacketQueue* audioQueue,AVPacketQueue* videoQueue)
    : m_audioQueue(audioQueue),m_videoQueue(videoQueue)
{
    LogInfo("Demuxthread()");
}

DemuxThread::~DemuxThread()
{
    LogInfo("~Demuxthread()");
}

/*
1. 分配格式上下文
2. 打开文件/流
3. 读取并分析流信息
4. 打印流信息（调试）
5. 查找音频/视频流索引
*/
int DemuxThread::init(const char *url)
{
    LogInfo("url:%s",url);
    int ret = 0;
    url_ = url;

    //分配并初始化一个 AVFormatContext 结构体。
    ifmt_ctx_ = avformat_alloc_context();

    //打开媒体文件或流（本地文件、HTTP、RTSP、摄像头等）。
    ret = avformat_open_input(&ifmt_ctx_,url_.c_str(),NULL,NULL);
    if(ret != 0){
        av_strerror(ret,err2str,sizeof (err2str));//把 FFmpeg 的错误码（int 类型） 转换成人类能看懂的 错误信息字符串。
        LogError("avformat_poen_input failed, ret:%d,err2str:%s",ret,err2str);
        return -1;
    }

    //作用：进一步读取媒体文件的头部信息，分析流（stream）的结构。
    ret = avformat_find_stream_info(ifmt_ctx_,NULL);
    if(ret != 0){
        av_strerror(ret,err2str,sizeof (err2str));//把 FFmpeg 的错误码（int 类型） 转换成人类能看懂的 错误信息字符串。
        LogError("avformat_poen_input failed, ret:%d,err2str:%s",ret,err2str);
        return -1;
    }

    //作用：把 ifmt_ctx_ 解析到的媒体文件信息打印到控制台。
    av_dump_format(ifmt_ctx_,0,url_.c_str(),0);

    //作用：自动选择一个“最合适的”音视频流。
    audio_index_ = av_find_best_stream(ifmt_ctx_,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    video_index_ = av_find_best_stream(ifmt_ctx_,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
    if(audio_index_ < 0 || video_index_ < 0){
        LogError("no audio or no video");
        return -1;
    }
    LogInfo("audio_index_:%d ,video_index_:%d",audio_index_,video_index_);

    LogInfo("Init finish");
    return 0;
}

int DemuxThread::start()
{
    thread_ = new std::thread(&DemuxThread::run,this);//创建线程并运行run
    if(!thread_){
        LogError("DemuxThread.start() failed");
        return -1;
    }
    return 0;
}

int DemuxThread::stop()
{
    Thread::stop();
    avformat_close_input(&ifmt_ctx_);
    return 0;
}

void DemuxThread::run()
{
    LogInfo("Run into");
    int ret = 0;
    AVPacket *pkt = av_packet_alloc();

    while(abort_ != 1){
        /*
         * demuxthread 在 audio/video 队列超过 100 时暂停，
         * 给下游解码线程时间消化已有数据，避免“供过于求”。
         */
        if(m_audioQueue->size() > 100 || m_videoQueue->size() > 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        ret = av_read_frame(ifmt_ctx_,pkt);//压缩数据未解码
        if(ret < 0){
            av_strerror(ret,err2str,sizeof (err2str));
            LogError("av_read_frame failed ,ret:%d,err2str:%s",ret,err2str);
            break; 
        }

        if(pkt->stream_index == audio_index_){
            m_audioQueue->push(pkt);
            //LogInfo("audio pkt , size: %d",m_audioQueue->size());
            //av_packet_unref(pkt);
        }else if(pkt->stream_index == video_index_){
            m_videoQueue->push(pkt);
            //LogInfo("video pkt , size: %d",m_videoQueue->size());
            //av_packet_unref(pkt);
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
    LogInfo("DemuxThread run finished!");
}

//获得参数CodecParameters
//传入函数avcodec_parameters_to_context(codec_ctx_,par);
AVCodecParameters *DemuxThread::AudioCodecParameters()
{
    if(audio_index_ != -1){
        return ifmt_ctx_->streams[audio_index_]->codecpar;
    }else{
        return nullptr;
    }
}

AVCodecParameters *DemuxThread::VideoCodecParameters()
{
    if(video_index_ != -1){
        return ifmt_ctx_->streams[video_index_]->codecpar;
    }else{
        return nullptr;
    }
}

AVRational DemuxThread::VideoStreamTimebase()
{
    if(video_index_ != -1) {
            return ifmt_ctx_->streams[video_index_]->time_base;
        } else {
            return AVRational{0, 0};
        }
}
