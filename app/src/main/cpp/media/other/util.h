//
// Created by 宋林涛 on 2018/10/22.
//
#pragma once

#define ERROR_OPEN_FILE -1
#define ERROR_FIND_STREAM -2
#define ERROR_AUDIO_DECODEC_EXCEPTION -3
#define ERROR_VIDEO_DECODEC_EXCEPTION -4
#define ERROR_REDAD_EXCEPTION -5
#define ERROR_OPEN_HARD_CODEC -6
#define ERROR_SURFACE_NULL -7
#define ERROR_JNI -8
#define ERROR_PREPARE_FIAL -9
#define DATA_DURATION 100
#define DATA_NOW_PLAYING_TIME 101
#define DATA_BUFFER_TIME 102

#define ACTION_PLAY_PREPARE 1
#define ACTION_PLAY_PREPARED 2
#define ACTION_PLAY 3
#define ACTION_PLAY_PAUSE 4
#define ACTION_PLAY_SEEK 5
#define ACTION_PLAY_SEEK_OVER 6
#define ACTION_PLAY_LOADING 7
#define ACTION_PLAY_LOADING_OVER 8
#define ACTION_PLAY_STOP 9
#define ACTION_PLAY_FINISH 10

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <sstream>
#include "../../android/android_log.h"

int get_codec_context(AVCodecParameters *codecParam, AVCodecContext **avCodecContext);

template<typename T>
std::string to_string(T from) {
    std::ostringstream stream;
    stream << from;
    return stream.str();
}


void thread_wait(pthread_cond_t *__cond, pthread_mutex_t *__mutex, long ms);

long getCurrentTime();

template<typename T>
inline void ii_deletep(T **arg) {
    delete (*arg);
    *arg = NULL;
}
