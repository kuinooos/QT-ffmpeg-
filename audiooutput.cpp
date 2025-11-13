#include "audiooutput.h"
#include "log.h"
AudioOutput::AudioOutput(const AudioParams &audio_params,AVFrameQueue *frame_queue)
    :src_tgt_(audio_params) , frame_queue_(frame_queue)
{

}

AudioOutput::~AudioOutput()
{

}

FILE *dump_pcm = NULL;
void fill_audio_pcm(void *udata, Uint8 *stream, int len){
    // 1. 从frame queue读取解码后的PCM的数据，填充到stream
    // 2. len = 4000字节， 一个frame有6000字节， 一次读取了4000， 这个frame剩了2000字节
    AudioOutput *is = (AudioOutput*)udata;
    int audio_size = 0;
    int len1 = 0;

    if(!dump_pcm){
        dump_pcm = fopen("dump.pcm", "wb");
    }

    //LogInfo("fill pcm len:%d", len);

    while(len > 0){//SDL需要len字节才能播放
        if(is->audio_buf_index == is->audio_buf_size){//当前缓存的数据已经全部送出，需要从frame queue读取新的数据
            is->audio_buf_index = 0;
            AVFrame *frame = is->frame_queue_->pop(10);//读取解码后的PCM数据，可能会超时返回NULL
            if(frame){

                // 读到解码后的数据
                // 怎么判断要不要做重采样
                if( ((frame->format != is->dst_tgt_.fmt)
                        || (frame->sample_rate != is->dst_tgt_.freq)
                        ||  (static_cast<int64_t>(frame->channel_layout) != is->dst_tgt_.channel_layout))
                        && (!is->swr_ctx_)) {
                    is->swr_ctx_ = swr_alloc_set_opts(NULL,
                                                      is->dst_tgt_.channel_layout,
                                                      (enum AVSampleFormat)is->dst_tgt_.fmt,
                                                      is->dst_tgt_.freq,
                                                      frame->channel_layout,
                                                      (enum AVSampleFormat)frame->format,
                                                      frame->sample_rate,
                                                      0, NULL);
                    if (!is->swr_ctx_ || swr_init(is->swr_ctx_) < 0) {
                        LogError(
                               "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                               frame->sample_rate,
                               av_get_sample_fmt_name((enum AVSampleFormat)frame->format),
                               frame->channels,
                               is->dst_tgt_.freq,
                               av_get_sample_fmt_name((enum AVSampleFormat)is->dst_tgt_.fmt),
                               is->dst_tgt_.channels);
                        swr_free((SwrContext **)(&is->swr_ctx_));
                        return;
                    }
            }
            if(is->swr_ctx_){//重采样
                //准备输入输出指针
                const uint8_t **in = (const uint8_t **)frame->extended_data;
                uint8_t **out = &is->audio_buf1_;
                //计算输出样本数量
                //目标采样率为dst_tgt_.freq，frame的采样率为frame->sample_rate
                int out_samples = frame->nb_samples * is->dst_tgt_.freq/frame->sample_rate + 256;
                //估算重采样后所需的buffer大小
                int out_bytes = av_samples_get_buffer_size(NULL,is->dst_tgt_.channels,out_samples,is->dst_tgt_.fmt,0);
                if(out_bytes < 0){
                    LogError("av_samples_get_buffer_size failed");
                    return;
                }
                //分配输出区
                av_fast_malloc(&is->audio_buf1_, &is->audio_buf1_size, out_bytes);
                int len2 = swr_convert(is->swr_ctx_,out,out_samples,in,frame->nb_samples);
                /*
                 * 🔁 swr_convert() 做的事可以概括为：

                读取输入样本 (in)
                执行转换：
                采样率变换（Resampling）

                声道布局调整（Mixing）

                样本格式转换（如 float → int16）

                输出结果到 out

                返回输出的样本数（每声道）
                 */
                if(len2 <0) {
                   LogError("swr_convert failed");
                   return;
                }
                is->audio_buf_ = is->audio_buf1_;
                is->audio_buf_size = av_samples_get_buffer_size(NULL, is->dst_tgt_.channels, len2, is->dst_tgt_.fmt, 1);

            }else{//无需重采样
                //计算当前帧的音频数据大小（字节数）
                audio_size = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, (enum AVSampleFormat)frame->format, 1);
                //为音频数据分配或扩展一块足够大的缓冲区
                av_fast_malloc(&is->audio_buf1_, &is->audio_buf1_size, audio_size);
                is->audio_buf_ = is->audio_buf1_;
                is->audio_buf_size = audio_size;
                memcpy(is->audio_buf_, frame->data[0], audio_size);

            }
            av_frame_free(&frame);
        }else{//if(frame)
            // 没有读到解码后的数据
            is->audio_buf_ = NULL;
            is->audio_buf_size = 512;
            }
    }//if(is->audio_buf_index == is->audio_buf_size)
    len1 = is->audio_buf_size - is->audio_buf_index;
    if(len1 > len)
        len1 = len;
    if(!is->audio_buf_) {
        //如果当前没有有效的音频数据，用静音数据（0）填充
        memset(stream, 0, len1);
    } else {
        // 真正拷贝有效的数据
        memcpy(stream, is->audio_buf_ + is->audio_buf_index, len1);
        SDL_MixAudio(stream, is->audio_buf_ + is->audio_buf_index, len1, SDL_MIX_MAXVOLUME/8 );
        fwrite((uint8_t *)is->audio_buf_ + is->audio_buf_index, 1, len1, dump_pcm);
        fflush(dump_pcm);
    }

    // 当成功输出 len1 字节的音频时，更新播放时钟
    if (is->audio_buf_) {
       int bytes_per_sec = is->dst_tgt_.freq *
                           is->dst_tgt_.channels *
                           av_get_bytes_per_sample(is->dst_tgt_.fmt);
       if (bytes_per_sec > 0) {
           is->audio_clock_ += (double)len1 / bytes_per_sec;
       }
    }
    len -= len1;
    stream += len1;
    is->audio_buf_index += len1;
}//while
}

int AudioOutput::init()
{
    if(SDL_Init(SDL_INIT_AUDIO) != 0){
        LogError("SDL_Init failed!");
        return -1;
    }

    //SDL_AudioSpec描述[音频设备的参数]以及回调函数
    SDL_AudioSpec wanted_spec;
    wanted_spec.channels = 2;//只支持2通道的输出
    wanted_spec.freq = src_tgt_.freq;//采样率
    wanted_spec.format = AUDIO_S16SYS;//采样格式
    wanted_spec.silence = 0;//静音时的填充值
    wanted_spec.callback = fill_audio_pcm;
    wanted_spec.userdata = this;
    wanted_spec.samples = 1024;//采样数量

    int ret = SDL_OpenAudio(&wanted_spec,nullptr);//打开音频设备
    if(ret != 0){
        LogInfo("SDL_OpenAudio failed!");
        return -1;
    }

    dst_tgt_.channels = wanted_spec.channels;//通道数
    dst_tgt_.freq = wanted_spec.freq;
    dst_tgt_.channel_layout = av_get_default_channel_layout(2);//默认声道布局，2为立体声
    dst_tgt_.fmt = AV_SAMPLE_FMT_S16;
    dst_tgt_.frame_size = 1024;

    SDL_PauseAudio(0);//播放音频
    LogInfo("AudioOutput::Init() leave");

    return 0;
}

int AudioOutput::DeInit()
{
    SDL_PauseAudio(1);//暂停播放
    SDL_CloseAudio();//关闭系统音频设备
    LogInfo("AudioOutput::DeInit() leave");

    return 0;
}

double* AudioOutput::getAudioClock()
{
    return &audio_clock_;
}


