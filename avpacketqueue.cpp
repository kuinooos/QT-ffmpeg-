#include "avpacketqueue.h"

AVPacketQueue::AVPacketQueue()
{

}

AVPacketQueue::~AVPacketQueue()
{

}

void AVPacketQueue::abort()
{
    release();
    AVP_queue_.abort();
}

int AVPacketQueue::push(AVPacket *val)
{
    AVPacket *tmp_pkt = av_packet_alloc();
    av_packet_move_ref(tmp_pkt, val);
    return AVP_queue_.push(tmp_pkt); // 直接交给队列管理
}

void AVPacketQueue::release()
{
    AVPacket *val_rl = NULL;
    int ret = 0;
    while(true){
        ret = AVP_queue_.pop(val_rl,1);
        if(ret < 0){
            break;
        }else{
            av_packet_free(&val_rl);
            continue;
        }
    }
}

int AVPacketQueue::size()
{
    return AVP_queue_.size();
}

AVPacket *AVPacketQueue::front()
{
    AVPacket* pkt;
    int ret = AVP_queue_.front(pkt);
    if(ret != 0 ){
        LogError("AVPacketQueue::front() failed!");
    }
    return pkt;
}

AVPacket *AVPacketQueue::pop(const int timeout)
{
    int ret;
    AVPacket *pop_val;
    ret = AVP_queue_.pop(pop_val,timeout);
    if(ret == -1){
        LogInfo("abort 生效，终止");
        return nullptr;
    }else if(ret == -2){
        LogInfo("队列为空");
    }
    return pop_val;
}
