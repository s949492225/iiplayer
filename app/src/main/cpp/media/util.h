//
// Created by 宋林涛 on 2018/10/22.
//

#ifndef IIPLAYER_UTIL_H
#define IIPLAYER_UTIL_H

#define ERROR_OPEN_FILE -1
#define ERROR_FIND_STREAM -2
#define ERROR_AUDIO_DECODEC_EXCEPTION -3
#define ERROR_VIDEO_DECODEC_EXCEPTION -4
#define ERROR_REDAD_EXCEPTION -4
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

#include <sstream>
#include "../android/android_log.h"

int get_codec_context(AVCodecParameters *codecParam, AVCodecContext **avCodecContext);

template <typename T>
char *to_char_str(T from) {
    std::stringstream ss;
    std::string str;
    ss << from;
    ss >> str;
    return const_cast<char *>(str.c_str());
}

#endif //IIPLAYER_UTIL_H

void thread_wait(pthread_cond_t* __cond, pthread_mutex_t* __mutex,long ms);