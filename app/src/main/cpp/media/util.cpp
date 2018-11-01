//
// Created by 宋林涛 on 2018/10/23.
//

#include "util.h"

int get_codec_context(AVCodecParameters *codecParam, AVCodecContext **avCodecContext) {
    AVCodec *dec;
//    if (codecParam->codec_type == AVMEDIA_TYPE_VIDEO) {
//        switch (codecParam->codec_id) {
//            case AV_CODEC_ID_H264:
//                dec = avcodec_find_decoder_by_name("h264_mediacodec");//硬解码264
//                break;
//            case AV_CODEC_ID_MPEG4:
//                dec = avcodec_find_decoder_by_name("mpeg4_mediacodec");//硬解码mpeg4
//                break;
//            case AV_CODEC_ID_HEVC:
//                dec = avcodec_find_decoder_by_name("hevc_mediacodec");//硬解码265
//                break;
//            default:
//                dec = avcodec_find_decoder(codecParam->codec_id);//软解
//                break;
//        }
//    } else {
    dec = avcodec_find_decoder(codecParam->codec_id);//软解
//    }

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

