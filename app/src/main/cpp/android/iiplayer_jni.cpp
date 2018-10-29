#include <stdint.h>//
// Created by 宋林涛 on 2018/10/22.
//

#include "iiplayer_jni.h"
#include "android_log.h"
#include "../media/media_player.h"

#define  IIMediaPlayer "com/syiyi/player/IIMediaPlayer"

jfieldID get_player_native_player_field(JNIEnv *env) {
    jclass jcs = env->FindClass(IIMediaPlayer);
    return env->GetFieldID(jcs, "mNativePlayer", "J");
}

jobject get_player_sender(JNIEnv *env, jobject obj) {
    jclass jcs = env->FindClass(IIMediaPlayer);
    jfieldID send_id = env->GetFieldID(jcs, "mHandler", "Landroid/os/Handler;");
    jobject send_obj = env->GetObjectField(obj, send_id);
    return send_obj;
}

media_player *get_media_player(JNIEnv *env, jobject obj) {
    jfieldID jfd = get_player_native_player_field(env);
    return reinterpret_cast<media_player *>(env->GetLongField(obj, jfd));
}

void set_media_player(JNIEnv *env, jobject obj, media_player *player) {
    //release old
    media_player *old_player = get_media_player(env, obj);
    if (old_player != NULL) {
        if (old_player->msg_sender != NULL) {
            env->DeleteGlobalRef(static_cast<jobject>(old_player->msg_sender));
            old_player->msg_sender = NULL;
        }
        old_player->stop();
        delete old_player;
    }
    jfieldID jfd = get_player_native_player_field(env);

    if (player != NULL) {
        player->msg_sender = env->NewGlobalRef(get_player_sender(env, obj));
        env->SetLongField(obj, jfd, reinterpret_cast<jlong>(player));
    } else {
        env->SetLongField(obj, jfd, 0);
    }
}

/**
 * 在java层保存c class的指针引用
 * @param env
 * @param obj
 */
static void JNICALL nativeInit(JNIEnv *env, jobject obj) {
    media_player *old_player = get_media_player(env, obj);
    if (old_player) {
        old_player->stop();
    }
    media_player *player = new media_player();
    set_media_player(env, obj, player);
}

static void JNICALL nativeOpen(JNIEnv *env, jobject obj, jstring url) {
    const char *str_c = env->GetStringUTFChars(url, NULL);
    char *new_str = strdup(str_c);
    env->ReleaseStringUTFChars(url, str_c);

    media_player *player = get_media_player(env, obj);
    player->open(new_str);
}

static void JNICALL nativePlay(JNIEnv *env, jobject obj) {
    media_player *player = get_media_player(env, obj);
    if (player != NULL) {
        player->play();
    }
}

static void JNICALL nativePause(JNIEnv *env, jobject obj) {
    media_player *player = get_media_player(env, obj);
    if (player != NULL) {
        player->pause();
    }
}

static void JNICALL nativeResume(JNIEnv *env, jobject obj) {
    media_player *player = get_media_player(env, obj);
    if (player != NULL) {

        player->resume();
    }
}

static void JNICALL nativeStop(JNIEnv *env, jobject obj) {
    media_player *player = get_media_player(env, obj);
    if (player != NULL) {
        set_media_player(env, obj, NULL);
    }
}

//++ jni register ++//
static JavaVM *g_jvm = NULL;
static const JNINativeMethod g_methods[] = {
        {"nativeInit",   "()V",                   (void *) nativeInit},
        {"nativeOpen",   "(Ljava/lang/String;)V", (void *) nativeOpen},
        {"nativePlay",   "()V",                   (void *) nativePlay},
        {"nativePause",  "()V",                   (void *) nativePause},
        {"nativeResume", "()V",                   (void *) nativeResume},
        {"nativeStop",   "()V",                   (void *) nativeStop}
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void __unused *reserved) {
    JNIEnv *env = NULL;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "iiplayer_jni", "ERROR:GetEnv failed\n");
        return -1;
    }

    jclass cls = env->FindClass(IIMediaPlayer);
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

    g_jvm = NULL;
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


