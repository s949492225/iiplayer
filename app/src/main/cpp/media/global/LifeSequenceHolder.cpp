//
// Created by 宋林涛 on 2018/11/5.
//

#include <pthread.h>
#include "LifeSequenceHolder.h"

LifeSequenceHolder::LifeSequenceHolder() {
    pthread_cond_init(&mReadCond, NULL);
}

LifeSequenceHolder::~LifeSequenceHolder() {
    pthread_cond_destroy(&mReadCond);

    if (mVideoCodecParam != NULL) {
        avcodec_parameters_free(&mVideoCodecParam);
    }

    if (mAudioCodecCtx != NULL) {
        avcodec_free_context(&mAudioCodecCtx);
    }

    if (mAbsCtx != NULL) {
        av_bsf_free(&mAbsCtx);
    }

    if (mVideoCodecCtx != NULL) {
        avcodec_free_context(&mVideoCodecCtx);
    }

    if (mFormatCtx != NULL) {
        avformat_close_input(&mFormatCtx);
        avformat_free_context(mFormatCtx);
        mFormatCtx = NULL;
    }


    avformat_network_deinit();
}

AVRational LifeSequenceHolder::getAudioTimeBase() {
    return mFormatCtx->streams[mAudioStreamIndex]->time_base;
}

AVRational LifeSequenceHolder::getVideoTimeBase() {
    return mFormatCtx->streams[mVideoStreamIndex]->time_base;
}


