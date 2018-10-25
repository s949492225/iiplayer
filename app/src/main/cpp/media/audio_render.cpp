//
// Created by 宋林涛 on 2018/9/28.
//

#include "audio_render.h"

audio_render::audio_render(status *playStatus, AVCodecContext *codecContext,
                           AVRational timebase) {
    this->playStatus = playStatus;
    this->sample_rate = codecContext->sample_rate;
    this->audio_timebase = timebase;
    this->audio_frame_queue = new frame_queue(playStatus);
    out_buffer = static_cast<uint8_t *>(av_malloc(static_cast<size_t>(44100 * 4)));

    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    swrCtr = swr_alloc();
    swr_alloc_set_opts(swrCtr,
                       AV_CH_LAYOUT_STEREO,
                       AV_SAMPLE_FMT_S16,
                       codecContext->sample_rate,
                       codecContext->channel_layout,
                       codecContext->sample_fmt,
                       codecContext->sample_rate,
                       NULL, NULL);
    //初始化
    swr_init(swrCtr);
    out_channel_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
}

void *play_thread(void *data) {
    LOGD("音频播放线程开始,tid:%i\n", gettid())
    audio_render *audio = static_cast<audio_render *>(data);
    audio->create_player();
//    audio->resample_audio();
}

void audio_render::play() {
    pthread_create(&play_t, NULL, play_thread, this);
//    play_thread(this);
}


int audio_render::get_pcm_data() {
    data_size = 0;
//    FILE *fp_pcm = fopen("/sdcard/test.pcm", "wb");
    while (playStatus != NULL && !playStatus->exit) {

        if (audio_frame_queue->getQueueSize() == 0)//加载中
        {
            if (!playStatus->load) {
                playStatus->load = true;
//                todo
            }
            av_usleep(1000 * 100);
            continue;
        } else {
            if (playStatus->load) {
                playStatus->load = false;
//                todo
            }
        }
        AVFrame *frame = av_frame_alloc();
        int ret = audio_frame_queue->getFrame(frame);
        if (ret == 0) {

            if (frame->channels && frame->channel_layout == 0) {
                frame->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(
                        frame->channels));
            } else if (frame->channels == 0 && frame->channel_layout > 0) {
                frame->channels = av_get_channel_layout_nb_channels(frame->channel_layout);
            }

            int nb = swr_convert(swrCtr, &out_buffer, 44100 * 4,
                                 (const uint8_t **) frame->data, frame->nb_samples);

            data_size = nb * out_channel_nb * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

            now_time = frame->pts * av_q2d(audio_timebase);
            if (now_time < clock) {
                now_time = clock;
            }
            clock = now_time;
//            fwrite(out_buffer, 1, static_cast<size_t>(data_size), fp_pcm);
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
            break;
        } else {
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
            continue;
        }
    }
    return data_size;
}

void pcm_buffer_callback(SLAndroidSimpleBufferQueueItf buffer_queue_if, void *context) {
    audio_render *audio = (audio_render *) context;
    if (audio != NULL) {
        int bufferSize = audio->get_pcm_data();
        if (bufferSize > 0) {
            audio->clock += bufferSize / ((double) 44100 * 4);
            if (audio->clock - audio->last_time >= 0.1) {
                audio->last_time = audio->clock;
                //回调应用层

            }
            SLresult result = (*audio->buffer_queue)->Enqueue(audio->buffer_queue,
                                                              (char *) audio->out_buffer,
                                                              static_cast<SLuint32>(bufferSize));
            if (result != SL_RESULT_SUCCESS) {
                LOGE("音频渲染出错\n");
            }
        }
    }
}

void audio_render::create_player() {

    SLresult result;
    result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*engineObject)->Realize(engineObject\n");
    }
    (void) result;
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*engineObject)->Realize(engineObject\n");
    }
    (void) result;
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*engineObject)->GetInterface(engineObject\n");
    }
    (void) result;
    //第二步，创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mids, mreq);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*engineEngine)->CreateOutputMix\n");
    }
    (void) result;
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*outputMixObject)->Realize(outputMixObject\n");
    }
    (void) result;
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*outputMixObject)->GetInterface\n");
    }
    (void) result;
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
        if (result != SL_RESULT_SUCCESS) {
            LOGE("音频渲染出错 SetEnvironmentalReverbProperties\n");
        }
        (void) result;
    }
    (void) result;
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, 0};


    // 第三步，配置PCM格式信息
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};

    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//播放pcm格式的数据
            2,//2个声道（立体声）
            static_cast<SLuint32>(get_format_sample_rate(sample_rate)),//44100hz的频率
            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN//结束标志
    };
    SLDataSource slDataSource = {&android_queue, &pcm};


    const SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_PLAYBACKRATE};
    const SLboolean req[2] = {SL_RESULT_SUCCESS, SL_RESULT_SUCCESS};

    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &pcmPlayerObject, &slDataSource,
                                                &audioSnk, 2,
                                                ids, req);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 CreateAudioPlayer\n");
    }
    (void) result;
    //初始化播放器
    result = (*pcmPlayerObject)->Realize(pcmPlayerObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*pcmPlayerObject)->Realize(pcmPlayerObject\n");
    }
    (void) result;
//    得到接口后调用  获取Player接口
    result = (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_PLAY, &pcmPlayerPlay);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*pcmPlayerObject)->SL_IID_PLAY\n");
    }
    (void) result;
//    注册回调缓冲区 获取缓冲队列接口
    result = (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_BUFFERQUEUE, &buffer_queue);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*pcmPlayerObject)->SL_IID_BUFFERQUEUE\n");
    }
    (void) result;
    //缓冲接口回调
    result = (*buffer_queue)->RegisterCallback(buffer_queue, pcm_buffer_callback, this);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 RegisterCallback\n");
    }
    (void) result;
//    获取播放状态接口
    result = (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 SetPlayState\n");
    }
    (void) result;
    pcm_buffer_callback(buffer_queue, this);

}

int audio_render::get_format_sample_rate(int sample_rate) {
    int rate = 0;
    switch (sample_rate) {
        case 8000:
            rate = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            rate = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            rate = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            rate = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            rate = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            rate = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            rate = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            rate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            rate = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            rate = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            rate = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            rate = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            rate = SL_SAMPLINGRATE_192;
            break;
        default:
            rate = SL_SAMPLINGRATE_44_1;
    }
    return rate;
}


void audio_render::pause() {
    if (pcmPlayerPlay != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PAUSED);
    }
}

void audio_render::resume() {
    if (pcmPlayerPlay != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    }
}

audio_render::~audio_render() {
}
