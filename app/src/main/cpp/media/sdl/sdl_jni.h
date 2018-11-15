//
// Created by 宋林涛 on 2018/11/15.
//
#include <jni.h>

#ifndef IIPLAYER_SDL_JNI_H
#define IIPLAYER_SDL_JNI_H

extern "C" JNIEXPORT void JNICALL
Java_com_syiyi_player_sdl_MediaCodecSurface_onDraw(JNIEnv *env, jobject obj, jlong pSDL);

#endif //IIPLAYER_SDL_JNI_H
