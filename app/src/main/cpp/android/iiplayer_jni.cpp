#include <stdint.h>//
#include <android/native_window_jni.h>
// Created by 宋林涛 on 2018/10/22.
//

#include "iiplayer_jni.h"
#include "android_log.h"
#include "../media/MediaPlayer.h"

extern "C" {
#include <libavcodec/jni.h>
}


#define  IIMediaPlayer "com/syiyi/player/IIMediaPlayer"
#define  MediaCodecSurface "com/syiyi/player/sdl/MediaCodecSurface"

jfieldID get_player_native_player_field(JNIEnv *env) {
    jclass jcs = env->FindClass(IIMediaPlayer);
    return env->GetFieldID(jcs, "mNativePlayer", "J");
}


MediaPlayer *get_media_player(JNIEnv *env, jobject obj) {
    jfieldID jfd = get_player_native_player_field(env);
    return reinterpret_cast<MediaPlayer *>(env->GetLongField(obj, jfd));
}

void set_media_player(JNIEnv *env, jobject obj, MediaPlayer *player) {
    //release old
    MediaPlayer *old_player = get_media_player(env, obj);

    if (old_player != NULL) {
        old_player->stop();
        delete old_player;
    }

    jfieldID jfd = get_player_native_player_field(env);
    if (player != NULL) {
        env->SetLongField(obj, jfd, (jlong) player);
    } else {
        env->SetLongField(obj, jfd, 0);
    }
}

/**
 * 在java层保存c class的指针引用
 * @param env
 * @param obj
 */
static void JNICALL init(JNIEnv *env, jobject obj) {
    MediaPlayer *player = new MediaPlayer(get_jni_jvm(), env, obj);
    set_media_player(env, obj, player);
}

static void JNICALL nativeOpen(JNIEnv *env, jobject obj, jstring url) {
    init(env, obj);

    const char *str_c = env->GetStringUTFChars(url, NULL);
    char *new_str = strdup(str_c);
    env->ReleaseStringUTFChars(url, str_c);

    MediaPlayer *player = get_media_player(env, obj);
    player->open(new_str);
}

static void JNICALL nativePlay(JNIEnv *env, jobject obj) {
    MediaPlayer *player = get_media_player(env, obj);
    if (player != NULL) {
        player->play();
    }
}

static void JNICALL nativePause(JNIEnv *env, jobject obj) {
    MediaPlayer *player = get_media_player(env, obj);
    if (player != NULL) {
        player->pause();
    }
}

static void JNICALL nativeResume(JNIEnv *env, jobject obj) {
    MediaPlayer *player = get_media_player(env, obj);
    if (player != NULL) {
        player->resume();
    }
}

static void JNICALL nativeSeek(JNIEnv *env, jobject obj, jint sec) {
    MediaPlayer *player = get_media_player(env, obj);
    if (player != NULL) {
        player->seek(sec);
    }
}

static void JNICALL nativeStop(JNIEnv *env, jobject obj) {
    MediaPlayer *player = get_media_player(env, obj);
    if (player != NULL) {
        set_media_player(env, obj, NULL);
    }
}

static jstring JNICALL nativeGetInfo(JNIEnv *env, jobject obj, jstring name) {
    MediaPlayer *player = get_media_player(env, obj);
    if (player != NULL) {
        const char *cName = env->GetStringUTFChars(name, NULL);
        char *new_str = strdup(cName);
        env->ReleaseStringUTFChars(name, cName);
        return player->getInfo(new_str);
    } else {
        return get_jni_env()->NewStringUTF("0");
    }
}

static void JNICALL nativeOnSufaceAvail(JNIEnv *env, jobject obj, bool isOk) {
    MediaPlayer *player = get_media_player(env, obj);
    if (player != NULL) {
        if (isOk) {
            player->createNativeWindow(env);
        }
        player->onSuraceAvali(isOk);
    }
}

//++ jni register ++//
static JavaVM *g_jvm = NULL;
static jclass jcls_mediac_codec_surface = NULL;
static const JNINativeMethod g_methods[] = {
        {"nativeOpen",          "(Ljava/lang/String;)V",                  (void *) nativeOpen},
        {"nativePlay",          "()V",                                    (void *) nativePlay},
        {"nativePause",         "()V",                                    (void *) nativePause},
        {"nativeResume",        "()V",                                    (void *) nativeResume},
        {"nativeSeek",          "(I)V",                                   (void *) nativeSeek},
        {"nativeStop",          "()V",                                    (void *) nativeStop},
        {"nativeGetInfo",       "(Ljava/lang/String;)Ljava/lang/String;", (void *) nativeGetInfo},
        {"nativeOnSufaceAvail", "(Z)V",                                   (void *) nativeOnSufaceAvail}
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void __unused *reserved) {
    av_jni_set_java_vm(vm, reserved);

    JNIEnv *env = NULL;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "iiplayer_jni", "ERROR:GetEnv failed\n");
        return -1;
    }

    jclass cls = env->FindClass(IIMediaPlayer);
    jcls_mediac_codec_surface = static_cast<jclass>(env->NewGlobalRef(
            env->FindClass(MediaCodecSurface)));


    int ret = env->RegisterNatives(cls, g_methods, sizeof(g_methods) / sizeof(g_methods[0]));
    if (ret != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "iiplayer_jni", "ERROR :RegisterNative failed\n");
        return -1;
    }

    //for g_vim
    g_jvm = vm;
    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    __android_log_print(ANDROID_LOG_INFO, "native", "JNI_OnUnload");
    //release
    JNIEnv *env = get_jni_env();
    jclass cls = env->FindClass(IIMediaPlayer);
    env->UnregisterNatives(cls);
    env->DeleteLocalRef(cls);

    env->DeleteGlobalRef(jcls_mediac_codec_surface);
    g_jvm = NULL;
}

JNIEXPORT jclass get_mediacodec_surface(void) {
    return jcls_mediac_codec_surface;
}


JNIEXPORT JavaVM *get_jni_jvm(void) {
    return g_jvm;
}

JNIEXPORT JNIEnv *get_jni_env(void) {
    JNIEnv *env = NULL;
    if (g_jvm == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, "iiplayer_jni", "ERROR:get_jni_env g_jvm is null\n");
        return NULL;
    }
    int status = g_jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4);
    if (status < 0) {
        status = g_jvm->AttachCurrentThread(&env, NULL);
        if (status != JNI_OK) {
            __android_log_print(ANDROID_LOG_ERROR, "iiplayer_jni",
                                "ERROR:get_jni_env attach current thread failed\n");
            return NULL;
        }
    }

    return env;

}


