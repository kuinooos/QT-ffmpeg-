#include <iostream>
#include "log.h"
#include "demuxthread.h"
#include "avpacketqueue.h"
#include "avframequeue.h"
#include "decodethread.h"
#include "audiooutput.h"
#include "videooutput.h"

using namespace std;

//打开文件 → 解复用 → 解码 → 播放
int main()
{
    cout << "Hello World!" << endl;

    LogInit();

    AVPacketQueue audio_packet_Queue;
    AVPacketQueue video_packet_Queue;

    AVFrameQueue audio_frame_Queue;
    AVFrameQueue video_frame_Queue;
    int ret = -1;
    //解复用
    DemuxThread demux_thread(&audio_packet_Queue,&video_packet_Queue);
    ret = demux_thread.init("time.mp4");
    if(ret != 0){
        LogError("demux_thread.init failed");
        return -1;
    }

    ret = demux_thread.start();
    if(ret != 0){
        LogError("demux_thread.start failed");
        return -1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    DecodeThread decode1(&audio_packet_Queue,&audio_frame_Queue);
    ret = decode1.init(demux_thread.AudioCodecParameters());
    if(ret != 0){
        LogError("decode1.init(demux_thread1.AudioCodecParameters()) failed");
        return -1;
    }
    ret = decode1.start();
    if(ret != 0){
        LogError("decode1.start() failed");
        return -1;
    }

    DecodeThread decode2(&video_packet_Queue,&video_frame_Queue);
    ret = decode2.init(demux_thread.VideoCodecParameters());
    if(ret != 0){
        LogError("decode2.init(demux_thread2.VideoCodecParameters()) failed");
        return -1;
    }
    ret = decode2.start();
    if(ret != 0){
        LogError("decode2.start() failed");
        return -1;
    }

    //初始化audio输出
    AudioParams audio_params = {0};
    memset(&audio_params,0,sizeof(AudioParams));
    audio_params.channels = demux_thread.AudioCodecParameters()->channels;
    audio_params.channel_layout = demux_thread.AudioCodecParameters()->channel_layout;
    audio_params.fmt = (enum AVSampleFormat)demux_thread.AudioCodecParameters()->format;
    audio_params.freq = demux_thread.AudioCodecParameters()->sample_rate;//采样率
    audio_params.frame_size = demux_thread.AudioCodecParameters()->frame_size;

    AudioOutput *audio_output = new AudioOutput(audio_params,&audio_frame_Queue);
    SDL_SetMainReady();
    ret = audio_output->init();
    if(ret != 0){
        LogInfo("audio_output->init() failed!");
        return -1;
    }

    VideoOutput *video_output = new VideoOutput(&video_frame_Queue,demux_thread.VideoCodecParameters()->width
                                                ,demux_thread.VideoCodecParameters()->height,audio_output->getAudioClock(),demux_thread.VideoStreamTimebase());

    ret = video_output->init();
    if(ret != 0){
        LogInfo("video_output->init() failed!");
        return -1;
    }

    video_output->MainLoop();

    //休眠120秒
    std::this_thread::sleep_for(std::chrono::milliseconds(120*1000));

    demux_thread.stop();


    decode1.stop();
    LogInfo("decode1.stop()");

    decode2.stop();
    LogInfo("decode2.stop()");

    LogInfo("main finish");

    return 0;

}
