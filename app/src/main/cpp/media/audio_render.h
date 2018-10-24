//
// Created by 宋林涛 on 2018/9/28.
//

#ifndef VIDEODEMO_SAUDIO_H
#define VIDEODEMO_SAUDIO_H

#include "status.h"
#include <pthread.h>
#include "frame_queue.h"
#include <unistd.h>

extern "C" {
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>

};

class audio_render {
public:
    status *playStatus = NULL;
    int sampleRate = 0;
    uint8_t *buffer = NULL;
    int data_size = 0;
    AVRational audio_timebase;
    frame_queue *audio_frame_queue;
    double clock;//总的播放时长
    double now_time;//当前frame时间
    double last_time; //上一次调用时间
    int max_frame_queue_size = 40;

    pthread_t play_t;
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

    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf buffer_queue = NULL;

    audio_render(status *playStatus, int sampleRate, AVRational rational);

    ~audio_render();

    void play();

    void pause();

    void resume();

    void create_opensles();

    int get_format_sample_rate(int sample_rate);

    int resample_audio();
};

#endif //VIDEODEMO_SAUDIO_H
