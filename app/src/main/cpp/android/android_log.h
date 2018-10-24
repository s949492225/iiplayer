//
// Created by yangw on 2018-2-28.
//

#ifndef IIPLAYER_ANDROIDLOG_H
#define IIPLAYER_ANDROIDLOG_H

#include "android/log.h"

#define LOG_DEBUG true

#define LOGD(FORMAT, ...) __android_log_print(ANDROID_LOG_DEBUG,"iiplayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"iiplayer",FORMAT,##__VA_ARGS__);
#endif //IIPLAYER_ANDROIDLOG_H



