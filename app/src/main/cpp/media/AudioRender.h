//
// Created by 宋林涛 on 2018/9/28.
//

#pragma once

#include "Status.h"
#include <pthread.h>
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

class AudioRender {
private:
    Status *mStatus = NULL;
    int mSampleRate = 0;
    uint8_t *mOutBuffer = NULL;
    int mOutSize = 0;
    AVRational mTimebase;
    FrameQueue *mQueue = NULL;
    int mMaxQueueSize = 40;

    double mSumPlayedTime = 0;//总的播放时长
    double mNowTime = 0;//当前frame时间
    double mLastTime = 0; //上一次调用时间
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

    void initSwr(AVCodecContext *context);

    void playThread();

    void friend renderAudio(SLAndroidSimpleBufferQueueItf  __unused queue,void *data);

public:
    AudioRender(Status *status, AVCodecContext *codecContext);

    ~AudioRender();

    void play();

    void pause();

    void resume();

    bool isQueueFull();

    void notifyWait();

    void putFrame(AVFrame *frame);

    void clearQueue();

    void resetTime();
};




