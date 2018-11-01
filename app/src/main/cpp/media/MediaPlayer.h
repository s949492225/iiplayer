//
// Created by 宋林涛 on 2018/10/22.
//

#ifndef IIPLAYER_MEDIA_PALYER_H
#define IIPLAYER_MEDIA_PALYER_H

#include "../android/android_log.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include "util.h"
#include "Status.h"
#include "AudioRender.h"
#include  "VideoRender.h"
#include  "../android/iiplayer_jni.h"
#include <unistd.h>
#include "time.h"
#include "../android/CallJava.h"

extern "C" {
#include "libavutil/time.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}

class MediaPlayer {
private:
    std::thread *mReadThread = NULL;
    std::thread *mAudioDecodeThread = NULL;
    std::thread *mVideoDecodeThread = NULL;
    AVFormatContext *mFormatCtx = NULL;
    const char *mUrl = NULL;
    int mDuration = 0;
    //audio
    int mAudioStreamIndex = -1;
    AVCodecContext *mAudioCodecCtx = NULL;
    AudioRender *mAudioRender = NULL;
    //video
    int mVideoStreamIndex = -1;
    AVCodecContext *mVideoCodecCtx = NULL;
    VideoRender *mVideoRender = NULL;

    CallJava *mCallJava;

    void *mGLRender = NULL;

    int prepare();

    void readThread();

    void decodeAudio();

    void decodeVideo();

    int friend ioInterruptCallback(void *ctx);

public:
    Status *mStatus = NULL;
    double mPlayTime = 0;

    MediaPlayer(JavaVM *pVM, JNIEnv *pEnv, jobject pJobject);

    void open(const char *string);

    void play();

    void pause();

    void resume();

    void seek(int sec);

    void stop();

    void release();

    void handlerSeek();

    void sendMsg(bool isMain, int type);

    void sendMsg(bool isMain, int type, int data);

    CallJava* get();
};


#endif //IIPLAYER_MEDIA_PALYER_H
