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
#include <unistd.h>
#include "time.h"

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

    Status *mStatus = NULL;
    std::thread *mReadThread = NULL;
    std::thread *mAudioDecodeThread = NULL;

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
    void *mMsgSender = NULL;

    int prepare();

    void readThread();

    void decodeAudio();

    void sendMsg(int type);

    void sendJniMsg(int type) const;

    int friend ioInterruptCallback(void *ctx);

public:
    MediaPlayer();

    void open(const char *string);

    void play();

    void pause();

    void resume();

    void stop();

    void setMsgSender(jobject *sender);
    void deleteMsgSender();
};


#endif //IIPLAYER_MEDIA_PALYER_H
