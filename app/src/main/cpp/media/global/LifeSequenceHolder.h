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
}


class LifeSequenceHolder {
public:
    AVFormatContext *mFormatCtx = NULL;
    AVCodecContext *mAudioCodecCtx = NULL;
    AVCodecContext *mVideoCodecCtx = NULL;
    AVCodecParameters *mVideoCodecParam = NULL;
    AVBSFContext *mAbsCtx = NULL;
    int mAudioStreamIndex = -1;
    int mVideoStreamIndex = -1;
    pthread_cond_t mCondRead;
    ~LifeSequenceHolder();

    AVRational getAudioTimeBase();
    AVRational getVideoTimeBase();
};


