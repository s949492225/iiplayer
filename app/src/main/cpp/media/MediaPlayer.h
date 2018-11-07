//
// Created by 宋林涛 on 2018/10/22.
//

#pragma once

#include "../android/android_log.h"
#include "other/util.h"
#include "global/Status.h"
#include "render/AudioRender.h"
#include  "render/VideoRender.h"
#include  "../android/iiplayer_jni.h"
#include "../android/CallJava.h"
#include "reader/PacketReader.h"
#include "global/LifeSequenceHolder.h"
#include "decode/Decoder.h"

class MediaPlayer {
private:
    const char *mUrl = NULL;
    CallJava *mCallJava = NULL;
    LifeSequenceHolder *mHolder = NULL;
    Status *mStatus = NULL;
    PacketReader *mReader = NULL;
    BaseDecoder *mAudioDecoder = NULL;
    BaseDecoder *mVideoDecoder = NULL;
    AudioRender *mAudioRender = NULL;
    VideoRender *mVideoRender = NULL;

    void notifyWait();

public:
    double mClock = 0;
    double mDuration = 0;
    int mRotation = 0;
    int mWidth = 0;
    int mHeight = 0;
    bool isOnlySoft = false;

    MediaPlayer(JavaVM *pVM, JNIEnv *pEnv, jobject obj);

    void open(const char *string);

    void play();

    void pause();

    void resume();

    void seek(int sec);

    void stop();

    void release(bool b);

    void sendMsg(bool isMain, int type);

    void sendMsg(bool isMain, int type, int data);

    Status *getStatus();

public:

    BaseDecoder *getAudioDecoder();

    BaseDecoder *getVideoDecoder();

    AudioRender *getAudioRender();

    VideoRender *getVideoRender();

    void setVideoDecoder(BaseDecoder *decoder);

    void setAudioDecoder(BaseDecoder *decoder);

    void setAudioRender(AudioRender *render);

    void setVideoRender(VideoRender *render);

    CallJava *getCallJava();

    jstring getInfo(char *string);

    const char *getUrl();

    LifeSequenceHolder *getHolder();

};
