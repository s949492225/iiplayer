//
// Created by 宋林涛 on 2018/11/1.
//

#ifndef IIPLAYER_CALLJAVA_H
#define IIPLAYER_CALLJAVA_H


#define NO_ARG -10000

#include <jni.h>
#include <libavutil/frame.h>

#define DECODE_HARD 1
#define DECODE_SOFT 0

class CallJava {
private:
    JavaVM *mVm = NULL;
    JNIEnv *mEnv = NULL;
    jobject mObj;
    jclass mjcls;
    jmethodID mJmidSendMsg;
    jmethodID mJmidSetFrameData;
    jmethodID mJmidSetCodeType;
    jmethodID mJmidInitMediaCodec;
    jmethodID mJmidIsSupportHard;
    jmethodID mJmidDecodeAVPacket;
    jmethodID mJmidReleaseMediaCodec;
    jfieldID mJfidIsSoftOnly;
public:
    CallJava(JavaVM *vm, JNIEnv *env, jobject obj);

    ~CallJava();

    void sendMsg(bool isMain, int type, int data);

    void setFrameData(bool isMain, int width, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv);

    void setCodecType(int type);

    bool isSoftOnly(bool isMain);

    bool isSupportHard(bool isMain, const char *codecName);

    int initMediaCodec(bool isMain, char *codecName, int width, int height, int csd_0_size,
                        int csd_1_size, uint8_t *csd_0, uint8_t *csd_1);

    void decodeAVPacket(bool isMain, int size, uint8_t *data);

    void releaseMediaCodec(bool b);
};


#endif //IIPLAYER_CALLJAVA_H
