//
// Created by 宋林涛 on 2018/11/1.
//

#include "CallJava.h"
#include "android_log.h"

CallJava::CallJava(JavaVM *vm, JNIEnv *env, jobject obj) {
    mVm = vm;
    mEnv = env;
    mObj = mEnv->NewGlobalRef(obj);
    mjcls = env->GetObjectClass(obj);
    mJmidSendMsg = env->GetMethodID(mjcls, "sendMessage", "(Landroid/os/Message;)V");
    mJmidSetFrameData = env->GetMethodID(mjcls, "setFrameData", "(II[B[B[B)V");
    mJmidSetCodeType = env->GetMethodID(mjcls, "setCodecType", "(I)V");
}

CallJava::~CallJava() {
    mEnv->DeleteGlobalRef(mObj);
    if (mVm != NULL) {
        mVm = NULL;
    }
    if (mEnv != NULL) {
        mEnv = NULL;
    }
}

jobject get_player_handler(JNIEnv *env, jobject obj) {
    jclass jcs = env->GetObjectClass(obj);
    jfieldID send_id = env->GetFieldID(jcs, "mHandler", "Landroid/os/Handler;");
    jobject send_obj = env->GetObjectField(obj, send_id);
    return send_obj;
}

void CallJava::sendMsg(bool isMain, int type, int data) {
    JNIEnv *env;
    if (!isMain) {
        if (mVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("CallJava::sendMsg AttachCurrentThread ERROR ")
            return;
        }
    } else {
        env = mEnv;
    }
    jobject sender = get_player_handler(env, mObj);
    jclass cls = env->GetObjectClass(sender);
    //msg
    jmethodID obMsgMethod = env->GetMethodID(cls, "obtainMessage", "(I)Landroid/os/Message;");
    //get msg
    jobject msg = env->CallObjectMethod(sender, obMsgMethod, type);

    if (data != NO_ARG) {
        //set arg1=data
        jclass msgCls = env->GetObjectClass(msg);
        jfieldID arg1FiledId = env->GetFieldID(msgCls, "arg1", "I");
        env->SetIntField(msg, arg1FiledId, data);
    }

    //send
    env->CallVoidMethod(mObj, mJmidSendMsg, msg);

    env->DeleteLocalRef(msg);
    env->DeleteLocalRef(sender);

    if (!isMain) {
        mVm->DetachCurrentThread();
    }
}

void CallJava::setFrameData(bool isMain, AVFrame *yuvFrame) {
    JNIEnv *env;
    if (!isMain) {
        if (mVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("CallJava::sendMsg AttachCurrentThread ERROR ")
            return;
        }
    } else {
        env = mEnv;
    }

    int width = yuvFrame->width;
    int height = yuvFrame->height;

    jbyteArray y = env->NewByteArray(width * height);
    env->SetByteArrayRegion(y, 0, width * height,
                            reinterpret_cast<const jbyte *>(yuvFrame->data[0]));

    jbyteArray u = env->NewByteArray(width * height / 4);
    env->SetByteArrayRegion(u, 0, width * height / 4,
                            reinterpret_cast<const jbyte *>(yuvFrame->data[1]));

    jbyteArray v = env->NewByteArray(width * height / 4);
    env->SetByteArrayRegion(v, 0, width * height / 4,
                            reinterpret_cast<const jbyte *>(yuvFrame->data[2]));

    env->CallVoidMethod(mObj, mJmidSetFrameData, width, height, y, u, v);

    env->DeleteLocalRef(y);
    env->DeleteLocalRef(u);
    env->DeleteLocalRef(v);

    if (!isMain) {
        mVm->DetachCurrentThread();
    }
}

void CallJava::setCodecType(int type) {
    JNIEnv *env = NULL;
    mVm->AttachCurrentThread(&env, 0);
    env->CallVoidMethod(mObj, mJmidSetCodeType, type);
    mVm->DetachCurrentThread();
}
