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
    jobject mObj;
    jclass mjcls;
    jmethodID mJmidSendMsg;
    jmethodID mJmidSetFrameData;
    jmethodID mJmidSetCodeType;
public:
    CallJava(JavaVM *vm, JNIEnv *env, jobject obj);

    ~CallJava();

    void sendMsg(bool isMain, int type, int data);

    void setFrameData(bool isMain,int width, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv);

    void setCodecType(int type);
};


#endif //IIPLAYER_CALLJAVA_H
