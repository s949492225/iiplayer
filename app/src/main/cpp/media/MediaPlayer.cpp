//
// Created by 宋林涛 on 2018/10/22.
//

#include "MediaPlayer.h"

#define MAX_QUEUE_SIZE (1024*1024)

int ioInterruptCallback(void *ctx) {
    MediaPlayer *player = static_cast<MediaPlayer *>(ctx);
    if (player->mStatus->isExit) {
        return AVERROR_EOF;
    }
    return 0;
}


MediaPlayer::MediaPlayer(JavaVM *pVM, JNIEnv *pEnv, jobject obj) {
    mCallJava = new CallJava(pVM, pEnv, obj);
    pthread_mutex_init(&mMutexRead, NULL);
}

void MediaPlayer::open(const char *url) {
    if (LOG_DEBUG) {
        LOGD("media_player open url:%s\n", url)
    }
    this->mUrl = url;
    this->mStatus = new Status();
    mReadThread = new std::thread(std::bind(&MediaPlayer::readThread, this));
    mAudioDecoder = new AudioDecoder(this);
    mVideoDecoder = new VideoDecoder(this);
}


int MediaPlayer::prepare() {
    sendMsg(false, ACTION_PLAY_PREPARE);

    av_register_all();
    avformat_network_init();

    mFormatCtx = avformat_alloc_context();
    mFormatCtx->interrupt_callback.callback = ioInterruptCallback;
    mFormatCtx->interrupt_callback.opaque = this;

    int error = avformat_open_input(&mFormatCtx, mUrl, NULL, NULL);
    if (error != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open url %s", mUrl);
        }
        sendMsg(false, ERROR_OPEN_FILE);
        return -1;
    }

    if (avformat_find_stream_info(mFormatCtx, NULL) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not find steams from %s", mUrl);
        }
        sendMsg(false, ERROR_FIND_STREAM);
        return -1;
    }
    //流信息获取
    for (int i = 0; i < mFormatCtx->nb_streams; i++) {
        AVCodecParameters *parameters = mFormatCtx->streams[i]->codecpar;
        if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            //音频解码器
            if (i != -1) {
                int ret = get_codec_context(parameters, &mAudioCodecCtx);
                if (ret < 0) {
                    sendMsg(false, ERROR_AUDIO_DECODEC_EXCEPTION, mDuration);
                    return -1;
                }
                int64_t sumTime = mFormatCtx->duration;
                mAudioRender = new AudioRender(this, sumTime, mAudioCodecCtx,
                                               mFormatCtx->streams[i]->time_base);
                mAudioStreamIndex = i;
                mDuration = static_cast<int>(sumTime / AV_TIME_BASE);
                sendMsg(false, DATA_DURATION, mDuration);
            }
        } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (i != -1) {
                int ret = get_codec_context(parameters, &mVideoCodecCtx);
                if (ret < 0) {
                    sendMsg(false, ERROR_VIDEO_DECODEC_EXCEPTION, mDuration);
                    return -1;
                }
                mCallJava->setCodecType(0);
                mVideoRender = new VideoRender(this, mVideoCodecCtx,
                                               mFormatCtx->streams[i]->time_base);
                mVideoStreamIndex = i;
            }
        }
    }

    setMediaInfo();

    sendMsg(false, ACTION_PLAY_PREPARED);
    mAudioDecoder->start(mAudioCodecCtx);
    mVideoDecoder->start(mVideoCodecCtx);
    return 0;
}


void MediaPlayer::readThread() {
    if (LOG_DEBUG) {
        LOGD("读取线程启动成功,tid:%i\n", gettid());
    }
    int error_code = prepare();
    if (error_code < 0) {
        return;
    }
    //read packet
    while (mStatus != NULL && !mStatus->isExit && !mStatus->isPlayEnd) {

        if (mStatus->isEOF && !mStatus->isSeek) {
            pthread_mutex_lock(&mMutexRead);
            thread_wait(&mStatus->mCondRead, &mMutexRead, 10);
            pthread_mutex_unlock(&mMutexRead);
            continue;
        }

        if (mStatus->isSeek) {
            handlerSeek();
        }

        if (mStatus->isPause) {
            av_usleep(1000 * 10);
            continue;
        }
        bool isSizeOver =
                mStatus->mVideoQueue->getQueueSize() + mStatus->mAudioQueue->getQueueSize() >
                MAX_QUEUE_SIZE;
        bool isAudioNeed = mStatus->mAudioQueue->getQueueSize() > MAX_QUEUE_SIZE / 2;
        bool isVideoNeed = mStatus->mVideoQueue->getQueueSize() > MAX_QUEUE_SIZE / 2;
        if (isSizeOver && isAudioNeed && isVideoNeed) {
            pthread_mutex_lock(&mMutexRead);
            thread_wait(&mStatus->mCondRead, &mMutexRead, 10);
            pthread_mutex_unlock(&mMutexRead);
            continue;
        }

        AVPacket *packet = av_packet_alloc();
        int ret;
        if ((ret = av_read_frame(mFormatCtx, packet)) == 0) {
            if (packet->stream_index == mVideoStreamIndex) {
                mStatus->mVideoQueue->putPacket(packet);
            } else if (packet->stream_index == mAudioStreamIndex) {
                mStatus->mAudioQueue->putPacket(packet);
                checkBuffer(packet);
            } else {
                av_packet_free(&packet);
            }
        } else {
            av_packet_free(&packet);
            if ((ret == AVERROR_EOF || avio_feof(mFormatCtx->pb) == 0)) {
                mStatus->isEOF = true;
                continue;
            }
            if (mFormatCtx->pb && mFormatCtx->pb->error) {
                sendMsg(false, ERROR_REDAD_EXCEPTION);
                seekErrorPos(static_cast<int>(mClock));
            }
            pthread_mutex_lock(&mMutexRead);
            thread_wait(&mStatus->mCondRead, &mMutexRead, 10);
            pthread_mutex_unlock(&mMutexRead);
        }
    }
}

void MediaPlayer::checkBuffer(AVPacket *packet) {
    double cachedTime = packet->pts * av_q2d(mAudioRender->mTimebase);
    mCallJava->sendMsg(false, DATA_BUFFER_TIME, static_cast<int>(cachedTime));
}

void MediaPlayer::play() {
    if (mStatus && mAudioRender) {
        mStatus->isPause = false;
        mAudioRender->play();
        mVideoRender->play();
        sendMsg(true, ACTION_PLAY);
    }
}

void MediaPlayer::pause() {
    if (mStatus && mAudioRender) {
        mStatus->isPause = true;
        mAudioRender->pause();
        sendMsg(true, ACTION_PLAY_PAUSE);
    }
}

void MediaPlayer::resume() {
    if (mStatus && mAudioRender) {
        mStatus->isPause = false;
        mAudioRender->resume();
        sendMsg(true, ACTION_PLAY);
    }
}

void MediaPlayer::seek(int sec) {
    int rel = 0;
    if (sec < 0)
        rel = 0;
    if (rel > mDuration)
        rel = mDuration;
    else
        rel = sec;
    mStatus->mSeekSec = rel;
    mStatus->isSeek = true;
    sendMsg(true, ACTION_PLAY_SEEK);
    pthread_cond_signal(&mStatus->mCondRead);
}

/**
 * 网络波动下自动修复
 * */
void MediaPlayer::seekErrorPos(int sec) {
    int rel = 0;
    if (sec < 0)
        rel = 0;
    if (rel > mDuration)
        rel = mDuration;
    else
        rel = sec;
    mStatus->mSeekSec = rel;
    mStatus->isSeek = true;
    pthread_cond_signal(&mStatus->mCondRead);
}

void MediaPlayer::handlerSeek() {
    //seek io
    int64_t rel = mStatus->mSeekSec * AV_TIME_BASE;
    int ret = avformat_seek_file(mFormatCtx, -1, INT64_MIN, rel, INT64_MAX, AVSEEK_FLAG_BACKWARD);
    if (ret == 0) {
        //clear
        mStatus->mAudioQueue->clearAll();
        mStatus->mVideoQueue->clearAll();

        mAudioRender->clearQueue();
        mVideoRender->clearQueue();
        mClock = mStatus->mSeekSec;
    } else {
        LOGE("seek fail")
    }
    mStatus->isEOF = false;
    mStatus->isSeek = false;
    sendMsg(false, ACTION_PLAY_SEEK_OVER);
}

void MediaPlayer::stop() {
    sendMsg(true, ACTION_PLAY_STOP);
    release();

}

void MediaPlayer::release() {

    if (mStatus) {
        mStatus->isLoad = false;
        mStatus->isExit = true;
        pthread_cond_signal(&mStatus->mCondRead);
        mStatus->mAudioQueue->notifyAll();
        mStatus->mVideoQueue->notifyAll();
    }

    if (mAudioRender) {
        mAudioRender->notifyWait();
    }

    if (mVideoRender) {
        mVideoRender->notifyWait();
    }


    if (mReadThread) {
        mReadThread->join();
        delete mReadThread;
        mReadThread = NULL;
    }

    if (mAudioDecoder != NULL) {
        delete mAudioDecoder;
        mAudioDecoder = NULL;
    }


    if (mVideoDecoder != NULL) {
        delete mVideoDecoder;
        mVideoDecoder = NULL;
    }


    if (mAudioRender != NULL) {
        delete mAudioRender;
        mAudioRender = NULL;
    }

    if (mVideoRender != NULL) {
        delete mVideoRender;
        mVideoRender = NULL;
    }

    if (mStatus != NULL) {
        delete mStatus;
        mStatus = NULL;
    }

    if (mAudioCodecCtx != NULL) {
        avcodec_close(mAudioCodecCtx);
        avcodec_free_context(&mAudioCodecCtx);
        mAudioCodecCtx = NULL;
    }

    if (mVideoCodecCtx != NULL) {
        avcodec_close(mVideoCodecCtx);
        avcodec_free_context(&mVideoCodecCtx);
        mVideoCodecCtx = NULL;
    }

    if (mFormatCtx != NULL) {
        avformat_close_input(&mFormatCtx);
        avformat_free_context(mFormatCtx);
        mFormatCtx = NULL;
    }

    avformat_network_deinit();

    if (mCallJava != NULL) {
        delete mCallJava;
    }

    if (LOG_DEBUG) {
        LOGD("player 资源释放完成");
    }
}

void MediaPlayer::sendMsg(bool isMain, int type, int data) {
    switch (type) {
        case
            ERROR_OPEN_FILE
            ERROR_FIND_STREAM
            ERROR_AUDIO_DECODEC_EXCEPTION
            ERROR_VIDEO_DECODEC_EXCEPTION
            ERROR_REDAD_EXCEPTION:
            release();
            break;
        case ACTION_PLAY_FINISH:
            release();
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
    return get_jni_env()->NewStringUTF("");;
}

void MediaPlayer::setMediaInfo() {
    //rotation
    AVDictionaryEntry *tag = NULL;
    tag = av_dict_get(mFormatCtx->streams[mVideoStreamIndex]->metadata, "rotate", tag, 0);
    if (tag == NULL) {
        mRotation = 0;
    } else {
        int angle = atoi(tag->value);
        angle %= 360;
        mRotation = angle;
    }

    mWidth = mVideoCodecCtx->width;
    mHeight = mVideoCodecCtx->height;
}

AudioRender *MediaPlayer::getAudioRender() {
    return mAudioRender;
}

VideoRender *MediaPlayer::getVideoRender() {
    return mVideoRender;
}



