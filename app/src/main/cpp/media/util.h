//
// Created by 宋林涛 on 2018/10/22.
//

#ifndef IIPLAYER_UTIL_H
#define IIPLAYER_UTIL_H

#define ERROR_OPEN_FILE -1
#define ERROR_FIND_STREAM -2
#define DATA_DURATION 100
#define DATA_NOW_PLAYING_TIME 101

#define ACTION_PLAY_PREPARE 1
#define ACTION_PLAY_PREPARED 2
#define ACTION_PLAY 3
#define ACTION_PLAY_PAUSE 4
#define ACTION_PLAY_SEEK 5
#define ACTION_PLAY_LOADING 6
#define ACTION_PLAY_LOADING_OVER 7
#define ACTION_PLAY_STOP 8
#define ACTION_PLAY_FINISH 9

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "../android/android_log.h"

int get_codec_context(AVCodecParameters *codecParam, AVCodecContext **avCodecContext);

#endif //IIPLAYER_UTIL_H
