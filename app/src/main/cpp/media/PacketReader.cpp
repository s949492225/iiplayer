//
// Created by 宋林涛 on 2018/11/5.
//

#include "PacketReader.h"
#include "MediaPlayer.h"
#include "AudioDecoder.h"
#include "AudioRender.h"
#include "VideoDecoder.h"
#include "VideoRender.h"
#include <unistd.h>

#define MAX_QUEUE_SIZE (1024*1024)

PacketReader::PacketReader(MediaPlayer *player) {
    mPlayer = player;
    mStatus = player->mStatus;
    pthread_mutex_init(&mMutexRead, NULL);
    pthread_cond_init(&mCondContinue, NULL);
    mAudioQueue = new PacketQueue(mStatus,mCondContinue, const_cast<char *>("audio"));
    mVideoQueue = new PacketQueue(mStatus,mCondContinue, const_cast<char *>("video"));
    mReadThread = new std::thread(std::bind(&PacketReader::read, this));

}

PacketReader::~PacketReader() {
    pthread_cond_signal(&mCondContinue);
    mReadThread->join();
    mReadThread = NULL;
    pthread_mutex_destroy(&mMutexRead);
    pthread_cond_destroy(&mCondContinue);

    if (mFormatCtx != NULL) {
        avformat_close_input(&mFormatCtx);
        avformat_free_context(mFormatCtx);
        mFormatCtx = NULL;
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


    avformat_network_deinit();
}


int ioInterruptCallback(void *ctx) {
    MediaPlayer *player = static_cast<MediaPlayer *>(ctx);
    if (player->mStatus->isExit) {
        return AVERROR_EOF;
    }
    return 0;
}

int PacketReader::prepare() {
    mPlayer->sendMsg(false, ACTION_PLAY_PREPARE);

    av_register_all();
    avformat_network_init();

    mFormatCtx = avformat_alloc_context();
    mFormatCtx->interrupt_callback.callback = ioInterruptCallback;
    mFormatCtx->interrupt_callback.opaque = mPlayer;
    const char *url = mPlayer->getUrl();
    int error = avformat_open_input(&mFormatCtx, url, NULL, NULL);
    if (error != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open url %s", url);
        }
        mPlayer->sendMsg(false, ERROR_OPEN_FILE);
        return -1;
    }

    if (avformat_find_stream_info(mFormatCtx, NULL) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not find steams from %s", url);
        }
        mPlayer->sendMsg(false, ERROR_FIND_STREAM);
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
                    mPlayer->sendMsg(false, ERROR_AUDIO_DECODEC_EXCEPTION);
                    return -1;
                }
                int64_t sumTime = mFormatCtx->duration;
                mPlayer->mAudioRender = new AudioRender(mPlayer, sumTime, mAudioCodecCtx,
                                                        mFormatCtx->streams[i]->time_base);
                mAudioStreamIndex = i;
                mPlayer->mDuration = static_cast<int>(sumTime / AV_TIME_BASE);
                mPlayer->sendMsg(false, DATA_DURATION, mPlayer->mDuration);
            }
        } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (i != -1) {
                int ret = get_codec_context(parameters, &mVideoCodecCtx);
                if (ret < 0) {
                    mPlayer->sendMsg(false, ERROR_VIDEO_DECODEC_EXCEPTION, mPlayer->mDuration);
                    return -1;
                }
                mPlayer->mCallJava->setCodecType(0);
                mPlayer->mVideoRender = new VideoRender(mPlayer, mVideoCodecCtx,
                                                        mFormatCtx->streams[i]->time_base);
                mVideoStreamIndex = i;
            }
        }
    }

    setMediaInfo();

    mPlayer->sendMsg(false, ACTION_PLAY_PREPARED);
    mPlayer->mAudioDecoder->start(mAudioCodecCtx);
    mPlayer->mVideoDecoder->start(mVideoCodecCtx);
    return 0;
}


void PacketReader::read() {
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
            thread_wait(&mCondContinue, &mMutexRead, 10);
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
                mVideoQueue->getQueueSize() +mAudioQueue->getQueueSize() >
                MAX_QUEUE_SIZE;
        bool isAudioNeed =mAudioQueue->getQueueSize() > MAX_QUEUE_SIZE / 2;
        bool isVideoNeed = mVideoQueue->getQueueSize() > MAX_QUEUE_SIZE / 2;
        if (isSizeOver && isAudioNeed && isVideoNeed) {
            pthread_mutex_lock(&mMutexRead);
            thread_wait(&mCondContinue, &mMutexRead, 10);
            pthread_mutex_unlock(&mMutexRead);
            continue;
        }

        AVPacket *packet = av_packet_alloc();
        int ret;
        if ((ret = av_read_frame(mFormatCtx, packet)) == 0) {
            if (packet->stream_index == mVideoStreamIndex) {
                mVideoQueue->putPacket(packet);
            } else if (packet->stream_index == mAudioStreamIndex) {
               mAudioQueue->putPacket(packet);
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
                mPlayer->sendMsg(false, ERROR_REDAD_EXCEPTION);
                seekErrorPos(static_cast<int>(mPlayer->mClock));
            }
            pthread_mutex_lock(&mMutexRead);
            thread_wait(&mCondContinue, &mMutexRead, 10);
            pthread_mutex_unlock(&mMutexRead);
        }
    }
}


void PacketReader::handlerSeek() {
    //seek io
    int64_t rel = mStatus->mSeekSec * AV_TIME_BASE;
    int ret = avformat_seek_file(mFormatCtx, -1, INT64_MIN, rel, INT64_MAX, AVSEEK_FLAG_BACKWARD);
    if (ret == 0) {
        //clear
       mAudioQueue->clearAll();
        mVideoQueue->clearAll();

        mPlayer->mAudioRender->clearQueue();
        mPlayer->mVideoRender->clearQueue();
        mPlayer->mClock = mStatus->mSeekSec;
    } else {
        LOGE("seek fail")
    }
    mStatus->isEOF = false;
    mStatus->isSeek = false;
    mPlayer->sendMsg(false, ACTION_PLAY_SEEK_OVER);
}

void PacketReader::checkBuffer(AVPacket *packet) {
    double cachedTime = packet->pts * av_q2d(mPlayer->mAudioRender->mTimebase);
    mPlayer->mCallJava->sendMsg(false, DATA_BUFFER_TIME, static_cast<int>(cachedTime));
}

/**
 * 网络波动下自动修复
 * */
void PacketReader::seekErrorPos(int sec) {
    int rel = 0;
    if (sec < 0)
        rel = 0;
    if (rel > mPlayer->mDuration)
        rel = mPlayer->mDuration;
    else
        rel = sec;
    mStatus->mSeekSec = rel;
    mStatus->isSeek = true;
    pthread_cond_signal(&mCondContinue);
}

void PacketReader::setMediaInfo() {
    AVDictionaryEntry *tag = NULL;
    tag = av_dict_get(mFormatCtx->streams[mVideoStreamIndex]->metadata, "rotate", tag, 0);
    if (tag == NULL) {
        mPlayer->mRotation = 0;
    } else {
        int angle = atoi(tag->value);
        angle %= 360;
        mPlayer->mRotation = angle;
    }

    mPlayer->mWidth = mVideoCodecCtx->width;
    mPlayer->mHeight = mVideoCodecCtx->height;
}

void PacketReader::seek(int sec) {
    int rel = 0;
    if (sec < 0)
        rel = 0;
    if (rel > mPlayer->mDuration)
        rel = mPlayer->mDuration;
    else
        rel = sec;
    mStatus->mSeekSec = rel;
    mStatus->isSeek = true;
    mPlayer->sendMsg(true, ACTION_PLAY_SEEK);
    pthread_cond_signal(&mCondContinue);
}

void PacketReader::notifyWait() {
    pthread_cond_signal(&mCondContinue);
    mAudioQueue->notifyAll();
    mVideoQueue->notifyAll();
}
