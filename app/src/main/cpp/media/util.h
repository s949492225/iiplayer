//
// Created by 宋林涛 on 2018/10/22.
//

#ifndef IIPLAYER_UTIL_H
#define IIPLAYER_UTIL_H

#define ERROR_OPEN_FILE -1
#define ERROR_FIND_STREAM -2
#define SUCCESS_PREPARED 1
extern "C" {
#include <libavcodec/avcodec.h>
}

#include "jni.h"
#include "../android/android_log.h"

int get_codec_context(AVCodecParameters *codecpar, AVCodecContext **avCodecContext);

#endif //IIPLAYER_UTIL_H
