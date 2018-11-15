//
// Created by 宋林涛 on 2018/11/1.
//

#ifndef IIPLAYER_CALLJAVA_H
#define IIPLAYER_CALLJAVA_H


#define NO_ARG -10000

#include <jni.h>
#include <libavutil/frame.h>

class CallJava {
private:
    JavaVM *mVm = NULL;
    JNIEnv *mEnv = NULL;
    jobject mObj = NULL;
    jmethodID mJmidSendMsg = NULL;
    jmethodID mJmidInitMediaCodec = NULL;
    jmethodID mJmidIsSupportHard = NULL;
    jmethodID mJmidDecodeAVPacket = NULL;
    jmethodID mJmidReleaseMediaCodec = NULL;
    jfieldID mJfidIsSoftOnly = NULL;
public:
    CallJava(JavaVM *vm, JNIEnv *env, jobject obj);

    ~CallJava();

    void sendMsg(bool isMain, int type, int data);

    bool isSoftOnly(bool isMain);

    bool isSupportHard(bool isMain, const char *codecName);

    int initMediaCodec(bool isMain, jobject surface, char *codecName, int width, int height,
                       int csd_0_size,
                       int csd_1_size, uint8_t *csd_0, uint8_t *csd_1);

    void decodeAVPacket(bool isMain, int size, uint8_t *data);

    void release(bool isMain);

    jobject getSurface(bool isMain);
};


#endif //IIPLAYER_CALLJAVA_H
