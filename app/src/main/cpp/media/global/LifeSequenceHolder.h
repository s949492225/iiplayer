//
// Created by 宋林涛 on 2018/11/5.
//

#pragma once
extern "C" {
#include "libavutil/time.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <semaphore.h>
}


class LifeSequenceHolder {
public:
    LifeSequenceHolder();

    ~LifeSequenceHolder();

    AVFormatContext *mFormatCtx = NULL;
    AVCodecContext *mAudioCodecCtx = NULL;
    AVCodecContext *mVideoCodecCtx = NULL;
    AVCodecParameters *mVideoCodecParam = NULL;
    AVBSFContext *mAbsCtx = NULL;
    AVFrame *mFlushFrame;
    AVPacket *mFlushPkt;
    int mAudioStreamIndex = -1;
    int mVideoStreamIndex = -1;

    pthread_cond_t mReadCond;

    AVRational getAudioTimeBase();

    AVRational getVideoTimeBase();
};


