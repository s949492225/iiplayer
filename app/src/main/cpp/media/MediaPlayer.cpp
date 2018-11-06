//
// Created by 宋林涛 on 2018/10/22.
//

#include "MediaPlayer.h"

#define MAX_QUEUE_SIZE (1024*1024)

MediaPlayer::MediaPlayer(JavaVM *pVM, JNIEnv *pEnv, jobject obj) {
    mHolder = new LifeSequenceHolder();
    mCallJava = new CallJava(pVM, pEnv, obj);
}

void MediaPlayer::open(const char *url) {
    if (LOG_DEBUG) {
        LOGD("media_player open url:%s\n", url)
    }
    mUrl = url;
    mStatus = new Status();
    isOnlySoft = mCallJava->isSoftOnly(true);
    mReader = new PacketReader(this);
}


void MediaPlayer::play() {
    if (mStatus) {
        mStatus->isPause = false;
        if (mAudioRender) {
            mAudioRender->play();
        }
        if (mVideoRender) {
            mVideoRender->play();
        }
        sendMsg(true, ACTION_PLAY);
    }
}

void MediaPlayer::pause() {
    if (mStatus) {
        mStatus->isPause = true;
        if (mAudioRender) {
            mAudioRender->pause();
        }
        sendMsg(true, ACTION_PLAY_PAUSE);
    }
}

void MediaPlayer::resume() {
    if (mStatus) {
        mStatus->isPause = false;
        if (mAudioRender) {
            mAudioRender->resume();
        }
        sendMsg(true, ACTION_PLAY);
    }
}

void MediaPlayer::seek(int sec) {
    if (mReader) {
        mReader->seek(sec);
    }
}

void MediaPlayer::stop() {
    sendMsg(true, ACTION_PLAY_STOP);
    release(true);

}

void MediaPlayer::sendMsg(bool isMain, int type, int data) {
    switch (type) {
        case ERROR_OPEN_FILE:
        case ERROR_FIND_STREAM:
        case ERROR_AUDIO_DECODEC_EXCEPTION:
        case ERROR_VIDEO_DECODEC_EXCEPTION:
        case ERROR_REDAD_EXCEPTION:
        case ERROR_OPEN_HARD_CODEC:
        case ERROR_SURFACE_NULL:
        case ERROR_JNI:
            release(false);
            break;
        case ACTION_PLAY_FINISH:
            release(false);
            break;
        default:
            break;
    }

    if (mCallJava != NULL) {
        mCallJava->sendMsg(isMain, type, data);
    }
}

void MediaPlayer::sendMsg(bool isMain, int type) {
    sendMsg(isMain, type, NO_ARG);
}

CallJava *MediaPlayer::getCallJava() {
    return mCallJava;
}

jstring MediaPlayer::getInfo(char *name) {
    if (strcmp("duration", name) == 0) {
        return get_jni_env()->NewStringUTF(to_char_str(mDuration));
    } else if (strcmp(name, "rotation") == 0) {
        return get_jni_env()->NewStringUTF(to_char_str(mRotation));
    } else if (strcmp(name, "width") == 0) {
        return get_jni_env()->NewStringUTF(to_char_str(mWidth));
    } else if (strcmp(name, "height") == 0) {
        return get_jni_env()->NewStringUTF(to_char_str(mHeight));
    } else if (strcmp(name, "played_time") == 0) {
        return get_jni_env()->NewStringUTF(to_char_str(mClock));
    }
    return get_jni_env()->NewStringUTF("0");;
}


AudioRender *MediaPlayer::getAudioRender() {
    return mAudioRender;
}

VideoRender *MediaPlayer::getVideoRender() {
    return mVideoRender;
}

const char *MediaPlayer::getUrl() {
    return mUrl;
}

void MediaPlayer::notifyWait() {
    if (mReader != NULL) {
        mReader->notifyWait();
    }

    if (mAudioRender) {
        mAudioRender->notifyWait();
    }

    if (mVideoRender) {
        mVideoRender->notifyWait();
    }
}

void MediaPlayer::release(bool isMain) {

    if (mStatus) {
        mStatus->isLoad = false;
        mStatus->isExit = true;
    }

    notifyWait();
    ii_deletep(&mReader);
    ii_deletep(&mAudioDecoder);
    ii_deletep(&mVideoDecoder);
    ii_deletep(&mAudioRender);
    ii_deletep(&mVideoRender);
    mCallJava->releaseMediaCodec(isMain);
    ii_deletep(&mStatus);
    ii_deletep(&mHolder);
    ii_deletep(&mCallJava);

    if (LOG_DEBUG) {
        LOGD("player 资源释放完成");
    }
}



