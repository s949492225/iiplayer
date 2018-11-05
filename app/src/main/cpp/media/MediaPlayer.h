//
// Created by 宋林涛 on 2018/10/22.
//

#ifndef IIPLAYER_MEDIA_PALYER_H
#define IIPLAYER_MEDIA_PALYER_H

#include "../android/android_log.h"
#include "other/util.h"
#include "global/Status.h"
#include "render/AudioRender.h"
#include  "render/VideoRender.h"
#include  "../android/iiplayer_jni.h"
#include "../android/CallJava.h"
#include "decode/AudioDecoder.h"
#include "decode/VideoDecoder.h"
#include "reader/PacketReader.h"
#include "global/LifeSequenceHolder.h"

class MediaPlayer {
private:
    const char *mUrl = NULL;
    PacketReader *mReader = NULL;
    void notifyWait();

public:
    Status *mStatus = NULL;
    double mClock = 0;
    int mDuration = 0;
    int mRotation = 0;
    int mWidth = 0;
    int mHeight = 0;
    CallJava *mCallJava = NULL;
    LifeSequenceHolder *mHolder;
    AudioDecoder *mAudioDecoder = NULL;
    VideoDecoder *mVideoDecoder = NULL;
    AudioRender *mAudioRender = NULL;
    VideoRender *mVideoRender = NULL;

    MediaPlayer(JavaVM *pVM, JNIEnv *pEnv, jobject pJobject);

    void open(const char *string);

    void play();

    void pause();

    void resume();

    void seek(int sec);

    void stop();

    void release();

    void sendMsg(bool isMain, int type);

    void sendMsg(bool isMain, int type, int data);

    CallJava *getCallJava();

    jstring getInfo(char *string);

    AudioRender *getAudioRender();

    VideoRender *getVideoRender();

    const char *getUrl();
};


#endif //IIPLAYER_MEDIA_PALYER_H
