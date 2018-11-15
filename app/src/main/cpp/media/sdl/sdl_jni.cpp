//
// Created by 宋林涛 on 2018/11/15.
//

#include "sdl_jni.h"
#include "SDLVideo.h"

extern "C" JNIEXPORT void JNICALL
Java_com_syiyi_player_sdl_MediaCodecSurface_onDraw(JNIEnv *env, jobject obj, jlong pAddr) {
    if (pAddr != 0) {
        SDLVideo *pVidep = reinterpret_cast<SDLVideo *>(pAddr);
        pVidep->drawMediaCodec();
    }
}