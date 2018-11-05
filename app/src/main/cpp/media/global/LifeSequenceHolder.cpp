//
// Created by 宋林涛 on 2018/11/5.
//

#include <pthread.h>
#include "LifeSequenceHolder.h"

LifeSequenceHolder::~LifeSequenceHolder() {

    pthread_cond_destroy(&mCondRead);

    if (mFormatCtx != NULL) {
        avformat_close_input(&mFormatCtx);
        avformat_free_context(mFormatCtx);
        mFormatCtx = NULL;
    }


    if (mAudioCodecCtx != NULL) {
        avcodec_close(mAudioCodecCtx);
        avcodec_free_context(&mAudioCodecCtx);
        mAudioCodecCtx = NULL;
    }

    if (mVideoCodecCtx != NULL) {
        avcodec_close(mVideoCodecCtx);
        avcodec_free_context(&mVideoCodecCtx);
        mVideoCodecCtx = NULL;
    }

    avformat_network_deinit();
}
