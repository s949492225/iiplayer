//
// Created by 宋林涛 on 2018/9/28.
//

#include "audio_render.h"

audio_render::audio_render(status *playStatus, AVCodecContext *codecContext) {
    this->playStatus = playStatus;
    this->sample_rate = codecContext->sample_rate;
    this->audio_timebase = codecContext->time_base;
    this->audio_frame_queue = new frame_queue(playStatus);
    this->out_channel_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_buffer = static_cast<uint8_t *>(av_malloc(static_cast<size_t>(SAMPLE_SIZE)));
    init_swr(codecContext);
}

void audio_render::init_swr(AVCodecContext *pContext) {
    swrCtr = swr_alloc();
    swr_alloc_set_opts(swrCtr,
                       AV_CH_LAYOUT_STEREO,
                       AV_SAMPLE_FMT_S16,
                       pContext->sample_rate,
                       pContext->channel_layout,
                       pContext->sample_fmt,
                       pContext->sample_rate,
                       NULL, NULL);
    swr_init(swrCtr);
}

void *play_thread(void *data) {
    LOGD("音频播放线程开始,tid:%i\n", gettid())
    audio_render *audio = static_cast<audio_render *>(data);
    audio->create_player();
    return NULL;
}

void audio_render::play() {
    pthread_create(&play_t, NULL, play_thread, this);
}


int audio_render::get_pcm_data() {
    data_size = 0;
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

            int nb = swr_convert(swrCtr, &out_buffer, SAMPLE_SIZE,
                                 (const uint8_t **) frame->data, frame->nb_samples);

            data_size = nb * out_channel_nb * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

            now_time = frame->pts * av_q2d(audio_timebase);
            if (now_time < clock) {
                now_time = clock;
            }
            clock = now_time;
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
            audio->clock += bufferSize / ((double) SAMPLE_SIZE);
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
            static_cast<SLuint32>(sample_rate * 1000),//44100hz的频率
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
    if (play_t != -1) {
        pthread_join(play_t, NULL);
    }

    if (pcmPlayerObject != NULL) {
        (*pcmPlayerObject)->Destroy(pcmPlayerObject);
        pcmPlayerObject = NULL;
        pcmPlayerPlay = NULL;
        buffer_queue = NULL;
    }

    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }

    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

    if (out_buffer != NULL) {
        delete (out_buffer);
        out_buffer = NULL;
    }

    if (swrCtr != NULL) {
        swr_free(&swrCtr);
        swrCtr = NULL;
    }

    if (audio_frame_queue != NULL) {
        delete audio_frame_queue;
        audio_frame_queue = NULL;
    }
}
