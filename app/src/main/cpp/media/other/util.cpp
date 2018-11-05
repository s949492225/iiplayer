//
// Created by 宋林涛 on 2018/10/23.
//

#include "util.h"

int get_codec_context(AVCodecParameters *codecParam, AVCodecContext **avCodecContext) {
    AVCodec *dec = avcodec_find_decoder(codecParam->codec_id);
    if (!dec) {
        if (LOG_DEBUG) {
            LOGE("can not find decoder");
        }
        return -1;
    }

    *avCodecContext = avcodec_alloc_context3(dec);
    if (!*avCodecContext) {
        if (LOG_DEBUG) {
            LOGE("can not alloc new codectx");
        }
        return -1;
    }

    if (avcodec_parameters_to_context(*avCodecContext, codecParam) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not fill codectx");
        }
        return -1;
    }

    int ret = avcodec_open2(*avCodecContext, dec, 0);
    if (ret != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open decoder");
        }
        return -1;
    }

    return 0;
}

void thread_wait(pthread_cond_t *__cond, pthread_mutex_t *__mutex, long timeout_ms) {
    struct timespec abstime;
    abstime.tv_sec = time(NULL) + timeout_ms / 1000;
    abstime.tv_nsec = (timeout_ms % 1000) * 1000000;
    pthread_cond_timedwait(__cond, __mutex, &abstime);
}



