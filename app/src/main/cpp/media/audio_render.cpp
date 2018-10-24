//
// Created by 宋林涛 on 2018/9/28.
//

#include "audio_render.h"

audio_render::audio_render(status *playStatus, int sampleRate, AVRational timebase) {
    this->playStatus = playStatus;
    this->sampleRate = sampleRate;
    this->audio_timebase = timebase;
    this->audio_frame_queue = new frame_queue(playStatus);
    buffer = static_cast<uint8_t *>(av_malloc(static_cast<size_t>(sampleRate * 2 * 2)));
}

void *play_thread(void *data) {
    LOGD("音频播放线程开始,tid:%i\n", gettid())
    audio_render *audio = static_cast<audio_render *>(data);
    audio->create_opensles();
}

void audio_render::play() {
    pthread_create(&play_t, NULL, play_thread, this);
}

void pcm_buffer_callback(SLAndroidSimpleBufferQueueItf buffer_queue_if, void *context) {
    audio_render *audio = (audio_render *) context;
    if (audio != NULL) {
        int bufferSize = audio->resample_audio();
        if (bufferSize > 0) {
            audio->clock += bufferSize / ((double) (audio->sampleRate * 2 * 2));
            if (audio->clock - audio->last_time >= 0.1) {
                audio->last_time = audio->clock;
                //回调应用层

            }
            (*audio->buffer_queue)->Enqueue(audio->buffer_queue, (char *) audio->buffer,
                                     static_cast<SLuint32>(bufferSize));
        }
    }
}

void audio_render::create_opensles() {

    SLresult result;
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, 0);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("slCreateEngine failed.");
    }
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("(*slEngine)->Realize failed.");
    }
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("(*engineObject)->GetInterface failed.");
    }

    //第二步，创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mids, mreq);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("(*engineEngine)->CreateOutputMix failed.");
    }
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("(*outputMixObject)->Realize failed.");
    }
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
        if (result != SL_RESULT_SUCCESS) {
            LOGE("(*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties failed.");
        }
    } else {
        LOGE("(*outputMixObject)->GetInterface failed.");
    }
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, 0};


    // 第三步，配置PCM格式信息
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};

    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//播放pcm格式的数据
            2,//2个声道（立体声）
            static_cast<SLuint32>(get_format_sample_rate(sampleRate)),//44100hz的频率
            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN//结束标志
    };
    SLDataSource slDataSource = {&android_queue, &pcm};


    const SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_PLAYBACKRATE};
    const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &pcmPlayerObject, &slDataSource,
                                                &audioSnk, 2,
                                                ids, req);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("(*engineEngine)->CreateAudioPlayer failed.");
    }
    //初始化播放器
    result = (*pcmPlayerObject)->Realize(pcmPlayerObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("(*pcmPlayerObject)->Realize failed.");
    }
//    得到接口后调用  获取Player接口
    result = (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_PLAY, &pcmPlayerPlay);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("(*pcmPlayerObject)->GetInterface failed.");
    }
//    注册回调缓冲区 获取缓冲队列接口
    result = (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_BUFFERQUEUE, &buffer_queue);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("(*pcmPlayerObject)->GetInterface failed.");
    }
    //缓冲接口回调
    result = (*buffer_queue)->RegisterCallback(buffer_queue, pcm_buffer_callback, this);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("(*pcmBufferQueue)->RegisterCallback failed.");
    }
//    获取播放状态接口
    result = (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("(*pcmPlayerPlay)->SetPlayState failed.");
    }
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

int audio_render::resample_audio() {
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
        AVFrame *frame = audio_frame_queue->getFrame();
        if (frame != NULL) {

            if (frame->channels && frame->channel_layout == 0) {
                frame->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(
                        frame->channels));
            } else if (frame->channels == 0 && frame->channel_layout > 0) {
                frame->channels = av_get_channel_layout_nb_channels(frame->channel_layout);
            }

            SwrContext *swr_ctx;

            swr_ctx = swr_alloc_set_opts(
                    NULL,
                    AV_CH_LAYOUT_STEREO,
                    AV_SAMPLE_FMT_S16,
                    frame->sample_rate,
                    frame->channel_layout,
                    (AVSampleFormat) frame->format,
                    frame->sample_rate,
                    NULL, NULL
            );
            if (!swr_ctx || swr_init(swr_ctx) < 0) {
                av_frame_free(&frame);
                av_free(frame);
                frame = NULL;
                swr_free(&swr_ctx);
                continue;
            }

            int nb = swr_convert(
                    swr_ctx,
                    &buffer,
                    frame->nb_samples,
                    (const uint8_t **) frame->data,
                    frame->nb_samples);

            int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
            data_size = nb * out_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

            now_time = frame->pts * av_q2d(audio_timebase);
            if (now_time < clock) {
                now_time = clock;
            }
            clock = now_time;

            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
            swr_free(&swr_ctx);
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

    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }

    delete audio_frame_queue;
}
