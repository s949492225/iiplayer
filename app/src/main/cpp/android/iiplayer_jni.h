//
// Created by 宋林涛 on 2018/10/22.
//

#ifndef IIPLAYER_IIPLAYER_JNI_H
#define IIPLAYER_IIPLAYER_JNI_H

#include <jni.h>
#include "../media/media_player.h"

JNIEXPORT JavaVM *get_jni_jvm(void);

JNIEXPORT JNIEnv *get_jni_env(void);

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *, void __unused *);

#endif //IIPLAYER_IIPLAYER_JNI_H
