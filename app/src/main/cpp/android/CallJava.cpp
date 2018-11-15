//
// Created by 宋林涛 on 2018/11/1.
//

#include "CallJava.h"
#include "android_log.h"

CallJava::CallJava(JavaVM *vm, JNIEnv *env, jobject obj) {
    mVm = vm;
    mEnv = env;
    mObj = mEnv->NewGlobalRef(obj);
    jclass jcls = env->GetObjectClass(obj);
    mJmidSendMsg = env->GetMethodID(jcls, "sendMessage", "(Landroid/os/Message;)V");
    mJmidInitMediaCodec = env->GetMethodID(jcls, "initMediaCodec", "(Landroid/view/Surface;Ljava/lang/String;II[B[B)I");
    mJmidIsSupportHard = env->GetMethodID(jcls, "isSupportHard", "(Ljava/lang/String;)Z");
    mJmidDecodeAVPacket = env->GetMethodID(jcls, "decodeAVPacket", "(I[B)V");
    mJmidReleaseMediaCodec = env->GetMethodID(jcls, "releaseMediaCodec", "()V");
    mJfidIsSoftOnly = env->GetFieldID(jcls, "isSoftOnly", "Z");
}

CallJava::~CallJava() {
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

bool CallJava::isSoftOnly(bool isMain) {
    JNIEnv *jniEnv;
    if (!isMain) {
        if (mVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("CallJava::sendMsg AttachCurrentThread ERROR ")
            return false;
        }
    } else {
        jniEnv = mEnv;
    }

    bool isSoftOnly = jniEnv->GetBooleanField(mObj, mJfidIsSoftOnly);

    if (!isMain) {
        mVm->DetachCurrentThread();
    }

    return isSoftOnly;
}

jobject CallJava::getSurface(bool isMain) {
    JNIEnv *jniEnv;
    if (!isMain) {
        if (mVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("CallJava::sendMsg AttachCurrentThread ERROR ")
            return NULL;
        }
    } else {
        jniEnv = mEnv;
    }
    jfieldID fid = jniEnv->GetFieldID(jniEnv->GetObjectClass(mObj), "mSurface",
                                      "Landroid/view/Surface;");
    jobject obj = jniEnv->GetObjectField(mObj, fid);

    if (!isMain) {
        mVm->DetachCurrentThread();
    }

    return obj;
}

int
CallJava::initMediaCodec(bool isMain, jobject surface, char *codecName, int width, int height,
                         int csd_0_size,
                         int csd_1_size, uint8_t *csd_0, uint8_t *csd_1) {
    JNIEnv *jniEnv;
    if (!isMain) {
        if (mVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("CallJava::sendMsg AttachCurrentThread ERROR ")
            return -3;
        }
    } else {
        jniEnv = mEnv;
    }

    jbyteArray csd0 = jniEnv->NewByteArray(csd_0_size);
    jniEnv->SetByteArrayRegion(csd0, 0, csd_0_size, (jbyte *) csd_0);
    jbyteArray csd1 = jniEnv->NewByteArray(csd_1_size);
    jniEnv->SetByteArrayRegion(csd1, 0, csd_1_size, (jbyte *) csd_1);
    jstring name = jniEnv->NewStringUTF(codecName);
    int ret = jniEnv->CallIntMethod(mObj, mJmidInitMediaCodec, surface, name, width, height, csd0,
                                    csd1);

    jniEnv->DeleteLocalRef(name);
    jniEnv->DeleteLocalRef(csd0);
    jniEnv->DeleteLocalRef(csd1);

    if (!isMain) {
        mVm->DetachCurrentThread();
    }
    return ret;
}

bool CallJava::isSupportHard(bool isMain, const char *codecName) {
    bool support;
    JNIEnv *jniEnv;
    if (!isMain) {
        if (mVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("CallJava::sendMsg AttachCurrentThread ERROR ")
            return false;
        }
    } else {
        jniEnv = mEnv;
    }


    jstring type = jniEnv->NewStringUTF(codecName);
    support = jniEnv->CallBooleanMethod(mObj, mJmidIsSupportHard, type);
    jniEnv->DeleteLocalRef(type);

    if (!isMain) {
        mVm->DetachCurrentThread();
    }

    return support;
}

void CallJava::decodeAVPacket(bool isMain, int size, uint8_t *src_data) {
    JNIEnv *jniEnv;
    if (!isMain) {
        if (mVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("CallJava::sendMsg AttachCurrentThread ERROR ")
            return;
        }
    } else {
        jniEnv = mEnv;
    }

    jbyteArray data = jniEnv->NewByteArray(size);
    jniEnv->SetByteArrayRegion(data, 0, size, reinterpret_cast<const jbyte *>(src_data));
    jniEnv->CallVoidMethod(mObj, mJmidDecodeAVPacket, size, data);
    jniEnv->DeleteLocalRef(data);

    if (!isMain) {
        mVm->DetachCurrentThread();
    }
}

void CallJava::release(bool isMain) {
    JNIEnv *jniEnv;
    if (!isMain) {
        if (mVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("CallJava::sendMsg AttachCurrentThread ERROR ")
            return;
        }
    } else {
        jniEnv = mEnv;
    }
    jniEnv->CallVoidMethod(mObj, mJmidReleaseMediaCodec);

    mEnv->DeleteGlobalRef(mObj);
    if (!isMain) {
        mVm->DetachCurrentThread();
    }

}





