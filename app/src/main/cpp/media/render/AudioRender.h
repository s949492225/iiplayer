//
// Created by 宋林涛 on 2018/9/28.
//

#pragma once

#include "../global/Status.h"
#include "../queue/FrameQueue.h"
#include "../sdl/SDLAudio.h"
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

class MediaPlayer;

class AudioRender {
private:

    MediaPlayer *mPlayer = NULL;
    AVRational mTimebase;

    uint8_t *mOutBuffer = NULL;
    int mOutSize = 0;

    int64_t duration;
    FrameQueue *mQueue = NULL;
    int mMaxQueueSize = 4;
    int mOutChannelNum;
    SwrContext *mSwrCtx = NULL;
    std::thread *mPlayThread = NULL;

    SDLAudio *mAudio;

    int mySampleRate = 0;

    void createPlayer();

    int getPcmData();

    void initSwrCtx(AVCodecContext *context);

    void playThread();

    void friend renderAudioCallBack(SLAndroidSimpleBufferQueueItf  __unused queue, void *data);

public:
    AudioRender(MediaPlayer *status);

    ~AudioRender();

    void play();

    void pause();

    void resume();

    bool isQueueFull();

    void notifyWait();

    void putFrame(AVFrame *frame);

    void clearQueue();
};




