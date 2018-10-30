//
// Created by 宋林涛 on 2018/9/28.
//

#pragma once

#include "Status.h"
#include <pthread.h>
#include "frame_queue.h"
#include <unistd.h>
#include <assert.h>

extern "C" {
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>

};

#define SAMPLE_SIZE 44100*4

class audio_render {
public:
    Status *playStatus = NULL;
    int sample_rate = 0;
    uint8_t *out_buffer = NULL;
    int data_size = 0;
    AVRational audio_timebase;
    frame_queue *audio_frame_queue = NULL;
    double clock = 0;//总的播放时长
    double now_time = 0;//当前frame时间
    double last_time = 0; //上一次调用时间
    int max_frame_queue_size = 40;
    int out_channel_nb;
    SwrContext *swrCtr = NULL;
    pthread_t play_t = -1;
    // 引擎接口
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine = NULL;

    //混音器
    SLObjectItf outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    //pcm
    SLObjectItf pcmPlayerObject = NULL;
    SLPlayItf pcmPlayerPlay = NULL;
    //voice
    SLVolumeItf fdPlayerVolume = NULL;


    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf buffer_queue = NULL;

    audio_render(Status *playStatus, AVCodecContext *codecContext);

    ~audio_render();

    void play();

    void pause();

    void resume();

    void create_player();

    int get_pcm_data();

    void init_swr(AVCodecContext *pContext);
};

