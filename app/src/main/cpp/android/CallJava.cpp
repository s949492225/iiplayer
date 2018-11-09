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
    mJmidInitMediaCodec = env->GetMethodID(mjcls, "initMediaCodec", "(Ljava/lang/String;II[B[B)I");
    mJmidIsSupportHard = env->GetMethodID(mjcls, "isSupportHard", "(Ljava/lang/String;)Z");
    mJmidDecodeAVPacket = env->GetMethodID(mjcls, "decodeAVPacket", "(I[B)V");
    mJmidReleaseMediaCodec = env->GetMethodID(mjcls, "releaseMediaCodec", "()V");
    mJfidIsSoftOnly = env->GetFieldID(mjcls, "isSoftOnly", "Z");
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

void
CallJava::setFrameData(bool isMain, int width, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv) {
    JNIEnv *jniEnv;
    if (!isMain) {
        if (mVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("CallJava::sendMsg AttachCurrentThread ERROR ")
            return;
        }
    } else {
        jniEnv = mEnv;
    }

    jbyteArray y = jniEnv->NewByteArray(width * height);
    jniEnv->SetByteArrayRegion(y, 0, width * height, reinterpret_cast<const jbyte *>(fy));

    jbyteArray u = jniEnv->NewByteArray(width * height / 4);
    jniEnv->SetByteArrayRegion(u, 0, width * height / 4, reinterpret_cast<const jbyte *>(fu));

    jbyteArray v = jniEnv->NewByteArray(width * height / 4);
    jniEnv->SetByteArrayRegion(v, 0, width * height / 4, reinterpret_cast<const jbyte *>(fv));

    jniEnv->CallVoidMethod(mObj, mJmidSetFrameData, width, height, y, u, v);

    jniEnv->DeleteLocalRef(y);
    jniEnv->DeleteLocalRef(u);
    jniEnv->DeleteLocalRef(v);

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

int
CallJava::initMediaCodec(bool isMain, char *codecName, int width, int height, int csd_0_size,
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
    int ret = jniEnv->CallIntMethod(mObj, mJmidInitMediaCodec, name, width, height, csd0, csd1);

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





