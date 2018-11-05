//
// Created by 宋林涛 on 2018/10/30.
//

#ifndef IIPLAYER_VIDEORENDER_H
#define IIPLAYER_VIDEORENDER_H

#include <thread>
#include "../android/android_log.h"
#include "FrameQueue.h"
#include <unistd.h>
#include "util.h"
#include "Status.h"
#include "../android/iiplayer_jni.h"


extern "C" {
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h>
}

class MediaPlayer;

class VideoRender {
public:

    VideoRender(MediaPlayer *player, AVCodecContext *codecContext, AVRational timebase);

    ~VideoRender();

    Status *mStatus = NULL;
    MediaPlayer *mPlayer = NULL;
    AVPixelFormat mPixFmt;
    int mWidth = 0;
    int mHeight = 0;
    FrameQueue *mQueue = NULL;
    int mMaxQueueSize = 40;
    AVRational mTimebase;
    std::thread *mPlayThread = NULL;

    void putFrame(AVFrame *frame);

    bool isQueueFull();

    void play();

    void playThread();

    void clearQueue();

    void notifyWait();

    double getFrameDiffTime(AVFrame *avFrame);

};


#endif //IIPLAYER_VIDEORENDER_H
