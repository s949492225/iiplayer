//
// Created by 宋林涛 on 2018/10/23.
//

#include "util.h"

int get_codec_context(AVCodecParameters *codecpar, AVCodecContext **avCodecContext) {
    AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
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

    if (avcodec_parameters_to_context(*avCodecContext, codecpar) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not fill codectx");
        }
        return -1;
    }


    if (avcodec_open2(*avCodecContext, dec, 0) != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open decoder");
        }
        return -1;
    }

    return 0;
}
