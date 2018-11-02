//
// Created by 宋林涛 on 2018/9/28.
//

#pragma once

#include "Status.h"
#include "FrameQueue.h"
#include <unistd.h>
#include <assert.h>
#include <thread>

extern "C" {
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>

};
#define SAMPLE_SIZE 44100*4

class MediaPlayer;

class AudioRender {
private:
    Status *mStatus = NULL;
    int mSampleRate = 0;
    uint8_t *mOutBuffer = NULL;
    int mOutSize = 0;
    int64_t duration;
    FrameQueue *mQueue = NULL;
    int mMaxQueueSize = 40;
    int mOutChannelNum;
    SwrContext *mSwrCtx = NULL;
    std::thread *mPlayThread = NULL;
    // 引擎接口
    SLObjectItf mEngineObj = NULL;
    SLEngineItf mEngineIF = NULL;

    //混音器
    SLObjectItf mMixObj = NULL;
    SLEnvironmentalReverbItf mMixEnvReverbIF = NULL;
    SLEnvironmentalReverbSettings mReverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    //pcm player
    SLObjectItf mPlayerObj = NULL;
    SLPlayItf mPlayerIF = NULL;
    //voice
    SLVolumeItf mVolumeIF = NULL;

    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf mBufferQueue = NULL;


    void createPlayer();

    int getPcmData();

    void initSwrCtx(AVCodecContext *context);

    void playThread();

    void friend renderAudioCallBack(SLAndroidSimpleBufferQueueItf  __unused queue, void *data);

public:
    MediaPlayer *mPlayer = NULL;
    AVRational mTimebase;

    AudioRender(MediaPlayer *status, int64_t duration, AVCodecContext *codecContext,AVRational timebase);

    ~AudioRender();

    void play();

    void pause();

    void resume();

    bool isQueueFull();

    void notifyWait();

    void putFrame(AVFrame *frame);

    void clearQueue();
};




