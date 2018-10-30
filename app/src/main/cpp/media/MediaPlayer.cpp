//
// Created by 宋林涛 on 2018/10/22.
//

#include "MediaPlayer.h"
#import  "../android/iiplayer_jni.h"

#define NO_ARG -10000

int ioInterruptCallback(void *ctx) {
    MediaPlayer *player = static_cast<MediaPlayer *>(ctx);
    if (player->mStatus->isExit) {
        return AVERROR_EOF;
    }
    return 0;
}


MediaPlayer::MediaPlayer() {

}

void MediaPlayer::open(const char *url) {
    if (LOG_DEBUG) {
        LOGD("media_player open url:%s\n", url)
    }
    this->mUrl = url;
    this->mStatus = new Status();
    mReadThread = new std::thread(std::bind(&MediaPlayer::readThread, this));
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
    while (mStatus != NULL && !mStatus->isExit) {
        if (mStatus->isPause) {
            av_usleep(1000 * 100);
            continue;
        }
        AVPacket *packet = av_packet_alloc();
        if (av_read_frame(mFormatCtx, packet) == 0) {
            //音频
            if (packet->stream_index == mAudioStreamIndex) {
                while (!mStatus->isExit && mStatus->mAudioQueue->getQueueSize() >=
                                           mStatus->mMaxQueueSize) {
                    av_usleep(1000 * 5);
                }
                if (mStatus->isExit)
                    break;
                mStatus->mAudioQueue->putPacket(packet);
                //视频
            } else {
                av_packet_free(&packet);
                av_free(packet);
            }
        } else {
            //播放完成
            av_packet_free(&packet);
            av_free(packet);
            if (LOG_DEBUG) {
                LOGD("文件读取结束\n");
            }
            break;
        }
    }
}


void MediaPlayer::decodeAudio() {
    if (LOG_DEBUG) {
        LOGD("音频解码线程开始,tid:%i\n", gettid())
    }
    AVPacket *packet = NULL;
    AVFrame *audioFrame = NULL;
    int ret = 0;

    while (mStatus != NULL && !mStatus->isExit) {

        if (mStatus->isSeek) {
            handlerSeek();
        }

        if (mStatus->isPause) {
            av_usleep(1000 * 100);
            continue;
        }

        packet = av_packet_alloc();
        if (mStatus->mAudioQueue->getPacket(packet) != 0) {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            continue;
        }
        ret = avcodec_send_packet(mAudioCodecCtx, packet);
        if (ret != 0) {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            continue;
        }

        audioFrame = av_frame_alloc();
        ret = avcodec_receive_frame(mAudioCodecCtx, audioFrame);
        if (ret == 0) {

            if (audioFrame->channels && audioFrame->channel_layout == 0) {
                audioFrame->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(
                        audioFrame->channels));
            } else if (audioFrame->channels == 0 && audioFrame->channel_layout > 0) {
                audioFrame->channels = av_get_channel_layout_nb_channels(
                        audioFrame->channel_layout);
            }
            while (!mStatus->isExit && mAudioRender->isQueueFull()) {
                av_usleep(1000 * 5);
            }
            if (mStatus->isExit) {
                continue;
            }
            mAudioRender->putFrame(audioFrame);
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
        } else {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
        }
    }
}


int MediaPlayer::prepare() {
    sendMsg(ACTION_PLAY_PREPARE);

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
        sendMsg(ERROR_OPEN_FILE);
        return -1;
    }

    if (avformat_find_stream_info(mFormatCtx, NULL) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not find steams from %s", mUrl);
        }
        sendMsg(ERROR_FIND_STREAM);
        return -1;
    }
    //流信息获取
    for (int i = 0; i < mFormatCtx->nb_streams; i++) {
        AVCodecParameters *parameters = mFormatCtx->streams[i]->codecpar;
        if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            //音频解码器
            if (i != -1) {
                get_codec_context(parameters, &mAudioCodecCtx);
                int64_t sumTime = mFormatCtx->duration;
                mAudioRender = new AudioRender(this, sumTime, mAudioCodecCtx);
                mAudioStreamIndex = i;
                mDuration = static_cast<int>(sumTime / AV_TIME_BASE);
                sendMsg(DATA_DURATION, mDuration);
            }
        } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (i != -1) {
                get_codec_context(parameters, &mVideoCodecCtx);
                mVideoStreamIndex = i;
            }
        }
    }

    sendMsg(ACTION_PLAY_PREPARED);
    //start audio decode thread
    mAudioDecodeThread = new std::thread(std::bind(&MediaPlayer::decodeAudio, this));
    return 1;
}


void MediaPlayer::play() {
    if (mStatus && mAudioRender) {
        mStatus->isPause = false;
        mAudioRender->play();
        sendMsg(ACTION_PLAY);
    }
}

void MediaPlayer::pause() {
    if (mStatus && mAudioRender) {
        mStatus->isPause = true;
        mAudioRender->pause();
        sendMsg(ACTION_PLAY_PAUSE);
    }
}

void MediaPlayer::resume() {
    if (mStatus && mAudioRender) {
        mStatus->isPause = false;
        mAudioRender->resume();
        sendMsg(ACTION_PLAY);
    }
}

void MediaPlayer::seek(int sec) {
    int rel = 0;
    if (sec < 0)
        rel = 0;
    if (rel > mDuration)
        rel = mDuration;
    mStatus->mSeekSec = rel;
    mStatus->isSeek = true;
    sendMsg(ACTION_PLAY_SEEK);
}

void MediaPlayer::handlerSeek() {
    //clear
    mStatus->mAudioQueue->clearAll();
    mStatus->mVideoQueue->clearAll();
    mAudioRender->clearQueue();
    mAudioRender->resetTime();

    //seek io
    int64_t rel = mStatus->mSeekSec * AV_TIME_BASE;
    avformat_seek_file(mFormatCtx, -1, INT64_MIN, rel, INT64_MAX, 0);

    mStatus->isSeek = false;
    sendMsg(ACTION_PLAY);
}

void MediaPlayer::stop() {
    release();
    sendMsg(ACTION_PLAY_STOP);
}


void MediaPlayer::release() {

    if (mStatus) {
        mStatus->isLoad = false;
        mStatus->isExit = true;
        mStatus->mAudioQueue->notifyAll();
        mStatus->mVideoQueue->notifyAll();
    }

    if (mAudioRender) {
        mAudioRender->notifyWait();
    }

    if (mReadThread) {
        mReadThread->join();
        delete mReadThread;
        mReadThread = NULL;
    }
    if (mAudioDecodeThread) {
        mAudioDecodeThread->join();
        delete mAudioDecodeThread;
        mAudioDecodeThread = NULL;
    }
    if (mAudioRender != NULL) {
        delete mAudioRender;
        mAudioRender = NULL;
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

    if (mMsgSender != NULL) {
        get_jni_env()->DeleteGlobalRef(static_cast<jobject>(mMsgSender));
        mMsgSender = NULL;
    }

    if (LOG_DEBUG) {
        LOGD("player 资源释放完成");
    }
}

void MediaPlayer::setMsgSender(jobject *sender) {
    mMsgSender = *sender;
}

void MediaPlayer::sendMsg(int type, int data) {
    switch (type) {
        case
            ERROR_OPEN_FILE
            ERROR_FIND_STREAM:
            release();
            break;
        default:
            break;
    }

    if (mMsgSender != NULL) {
        sendJniMsg(type, data);
    }
}

void MediaPlayer::sendMsg(int type) {
    if (mMsgSender != NULL) {
        sendJniMsg(type, NO_ARG);
    }
}

void MediaPlayer::sendJniMsg(int type, int data) const {
    JNIEnv *env = get_jni_env();

    //handler
    jobject sender = static_cast<jobject>(mMsgSender);
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
    jmethodID send_method = env->GetMethodID(cls, "sendMessage", "(Landroid/os/Message;)Z");
    env->CallBooleanMethod(sender, send_method, msg);
}


