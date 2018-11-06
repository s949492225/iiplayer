//
// Created by 宋林涛 on 2018/11/5.
//

#include "PacketReader.h"
#include "../MediaPlayer.h"
#include "../decode/AudioDecoder.h"
#include "../render/AudioRender.h"
#include "../decode/VideoDecoder.h"
#include "../render/VideoRender.h"
#include "../decode/HardVideoDecoder.h"
#include <unistd.h>

#define MAX_QUEUE_SIZE (1024*1024)

PacketReader::PacketReader(MediaPlayer *player) {
    mPlayer = player;
    pthread_mutex_init(&mMutexRead, NULL);
    pthread_cond_init(&mPlayer->mHolder->mCondRead, NULL);
    mReadThread = new std::thread(std::bind(&PacketReader::read, this));

}

PacketReader::~PacketReader() {
    if (mReadThread) {
        mReadThread->join();
        mReadThread = NULL;
        pthread_mutex_destroy(&mMutexRead);
    }
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

    mPlayer->mHolder->mFormatCtx = avformat_alloc_context();
    mPlayer->mHolder->mFormatCtx->interrupt_callback.callback = ioInterruptCallback;
    mPlayer->mHolder->mFormatCtx->interrupt_callback.opaque = mPlayer;
    const char *url = mPlayer->getUrl();
    int error = avformat_open_input(&mPlayer->mHolder->mFormatCtx, url, NULL, NULL);
    if (error != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open url %s", url);
        }
        mPlayer->sendMsg(false, ERROR_OPEN_FILE);
        return -1;
    }

    if (avformat_find_stream_info(mPlayer->mHolder->mFormatCtx, NULL) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not find steams from %s", url);
        }
        mPlayer->sendMsg(false, ERROR_FIND_STREAM);
        return -1;
    }
    //流信息获取
    for (int i = 0; i < mPlayer->mHolder->mFormatCtx->nb_streams; i++) {
        AVCodecParameters *parameters = mPlayer->mHolder->mFormatCtx->streams[i]->codecpar;
        if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            //音频解码器
            if (i != -1) {
                int ret = get_codec_context(parameters, &mPlayer->mHolder->mAudioCodecCtx);
                if (ret < 0) {
                    mPlayer->sendMsg(false, ERROR_AUDIO_DECODEC_EXCEPTION);
                    return -1;
                }
                mPlayer->mHolder->mAudioStreamIndex = i;
                mPlayer->mDuration = mPlayer->mHolder->mFormatCtx->duration / AV_TIME_BASE;
                mPlayer->sendMsg(false, DATA_DURATION, static_cast<int>(mPlayer->mDuration));
            }
        } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (i != -1) {
                mPlayer->mHolder->mVideoCodecParam = parameters;
                int ret = get_codec_context(parameters, &mPlayer->mHolder->mVideoCodecCtx);
                if (ret < 0) {
                    mPlayer->sendMsg(false, ERROR_VIDEO_DECODEC_EXCEPTION,
                                     static_cast<int>(mPlayer->mDuration));
                    return -1;
                }
                mPlayer->mHolder->mVideoStreamIndex = i;

            }
        }
    }

    setMediaInfo();

    initBitStreamFilter();

    mPlayer->sendMsg(false, ACTION_PLAY_PREPARED);

    mPlayer->mAudioDecoder = new AudioDecoder(mPlayer);
    mPlayer->mAudioDecoder->init();
    mPlayer->mAudioRender = new AudioRender(mPlayer);

    if (isHardCodec()) {
        mPlayer->mCallJava->setCodecType(DECODE_HARD);
        mPlayer->mVideoDecoder = new HardVideoDecoder(mPlayer);
        mPlayer->mVideoDecoder->init();
    } else {
        mPlayer->mCallJava->setCodecType(DECODE_SOFT);
        mPlayer->mVideoDecoder = new VideoDecoder(mPlayer);
        mPlayer->mVideoRender = new VideoRender(mPlayer);
        mPlayer->mVideoDecoder->init();
    }
    return 0;
}

bool PacketReader::isHardCodec() const {
    return !mPlayer->isOnlySoft && mPlayer->mHolder->mAbsCtx != NULL;
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
    while (mPlayer->mStatus != NULL && !mPlayer->mStatus->isExit && !mPlayer->mStatus->isPlayEnd) {

        if (mPlayer->mStatus->isEOF && !mPlayer->mStatus->isSeek) {
            pthread_mutex_lock(&mMutexRead);
            thread_wait(&mPlayer->mHolder->mCondRead, &mMutexRead, 10);
            pthread_mutex_unlock(&mMutexRead);
            continue;
        }

        if (mPlayer->mStatus->isSeek) {
            handlerSeek();
        }

        if (mPlayer->mStatus->isPause) {
            av_usleep(1000 * 10);
            continue;
        }
        bool isSizeOver =
                mPlayer->mVideoDecoder->getQueueSize() +
                mPlayer->mAudioDecoder->getQueueSize() >
                MAX_QUEUE_SIZE;
        bool isAudioNeed = mPlayer->mAudioDecoder->getQueueSize() > MAX_QUEUE_SIZE / 2;
        bool isVideoNeed = mPlayer->mVideoDecoder->getQueueSize() > MAX_QUEUE_SIZE / 2;
        if (isSizeOver & isAudioNeed & isVideoNeed) {
            pthread_mutex_lock(&mMutexRead);
            thread_wait(&mPlayer->mHolder->mCondRead, &mMutexRead, 10);
            pthread_mutex_unlock(&mMutexRead);
            continue;
        }

        AVPacket *packet = av_packet_alloc();
        int ret;
        if ((ret = av_read_frame(mPlayer->mHolder->mFormatCtx, packet)) == 0) {

            if (packet->stream_index == mPlayer->mHolder->mVideoStreamIndex) {

                if (isHardCodec()) {
                    if (mPlayer->mHolder->mAbsCtx != NULL) {
                        if (av_bsf_send_packet(mPlayer->mHolder->mAbsCtx, packet) != 0) {
                            av_packet_free(&packet);
                            continue;
                        }
                        int recSuccess = 0;
                        while (recSuccess == 0) {
                            AVPacket *newPacket = av_packet_alloc();
                            recSuccess = av_bsf_receive_packet(mPlayer->mHolder->mAbsCtx,
                                                               newPacket);
                            mPlayer->mVideoDecoder->putPacket(newPacket);
                        }
                        av_packet_free(&packet);
                    }
                } else {
                    mPlayer->mVideoDecoder->putPacket(packet);
                }

            } else if (packet->stream_index == mPlayer->mHolder->mAudioStreamIndex) {
                mPlayer->mAudioDecoder->putPacket(packet);
                checkBuffer(packet);
            } else {
                av_packet_free(&packet);
            }

        } else {
            av_packet_free(&packet);
            if ((ret == AVERROR_EOF || avio_feof(mPlayer->mHolder->mFormatCtx->pb) == 0)) {
                mPlayer->mStatus->isEOF = true;
                continue;
            }
            if (mPlayer->mHolder->mFormatCtx->pb && mPlayer->mHolder->mFormatCtx->pb->error) {
                mPlayer->sendMsg(false, ERROR_REDAD_EXCEPTION);
                seekErrorPos(static_cast<int>(mPlayer->mClock));
            }
            pthread_mutex_lock(&mMutexRead);
            thread_wait(&mPlayer->mHolder->mCondRead, &mMutexRead, 10);
            pthread_mutex_unlock(&mMutexRead);
        }
    }
}

void PacketReader::initBitStreamFilter() {
    const AVBitStreamFilter *filter = NULL;
    const char *codecName = mPlayer->mHolder->mVideoCodecCtx->codec->name;
    if (mPlayer->mCallJava->isSupportHard(false, codecName)) {
        if (strcasecmp(codecName, "h264") == 0) {
            filter = av_bsf_get_by_name("h264_mp4toannexb");
        } else if (strcasecmp(codecName, "h265") == 0) {
            filter = av_bsf_get_by_name("hevc_mp4toannexb");
        }
    }

    if (filter == NULL) {
        return;
    }

    if (av_bsf_alloc(filter, &mPlayer->mHolder->mAbsCtx) != 0) {
        return;
    }
    if (avcodec_parameters_copy(mPlayer->mHolder->mAbsCtx->par_in,
                                mPlayer->mHolder->mVideoCodecParam) < 0) {
        av_bsf_free(&mPlayer->mHolder->mAbsCtx);
        mPlayer->mHolder->mAbsCtx = NULL;
        return;
    }
    if (av_bsf_init(mPlayer->mHolder->mAbsCtx) != 0) {
        av_bsf_free(&mPlayer->mHolder->mAbsCtx);
        mPlayer->mHolder->mAbsCtx = NULL;
        return;
    }
    mPlayer->mHolder->mAbsCtx->time_base_in = mPlayer->mHolder->mFormatCtx->streams[mPlayer->mHolder->mVideoStreamIndex]->time_base;

}


void PacketReader::handlerSeek() {
    //seek io
    int64_t rel = mPlayer->mStatus->mSeekSec * AV_TIME_BASE;
    int ret = avformat_seek_file(mPlayer->mHolder->mFormatCtx, -1, INT64_MIN, rel, INT64_MAX,
                                 AVSEEK_FLAG_BACKWARD);
    if (ret == 0) {
        //clear
        mPlayer->mAudioDecoder->clearQueue();
        mPlayer->mVideoDecoder->clearQueue();

        mPlayer->mAudioRender->clearQueue();
        if (mPlayer->mVideoRender) {
            mPlayer->mVideoRender->clearQueue();
        }
        mPlayer->mClock = mPlayer->mStatus->mSeekSec;
    } else {
        LOGE("seek fail")
    }
    mPlayer->mStatus->isEOF = false;
    mPlayer->mStatus->isSeek = false;
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
        rel = static_cast<int>(mPlayer->mDuration);
    else
        rel = sec;
    mPlayer->mStatus->mSeekSec = rel;
    mPlayer->mStatus->isSeek = true;
    pthread_cond_signal(&mPlayer->mHolder->mCondRead);
}

void PacketReader::setMediaInfo() {
    AVDictionaryEntry *tag = NULL;
    tag = av_dict_get(
            mPlayer->mHolder->mFormatCtx->streams[mPlayer->mHolder->mVideoStreamIndex]->metadata,
            "rotate", tag, 0);
    if (tag == NULL) {
        mPlayer->mRotation = 0;
    } else {
        int angle = atoi(tag->value);
        angle %= 360;
        mPlayer->mRotation = angle;
    }

    mPlayer->mWidth = mPlayer->mHolder->mVideoCodecCtx->width;
    mPlayer->mHeight = mPlayer->mHolder->mVideoCodecCtx->height;
}

void PacketReader::seek(int sec) {
    int rel = 0;
    if (sec < 0)
        rel = 0;
    if (rel > mPlayer->mDuration)
        rel = static_cast<int>(mPlayer->mDuration);
    else
        rel = sec;
    mPlayer->mStatus->mSeekSec = rel;
    mPlayer->mStatus->isSeek = true;
    mPlayer->sendMsg(true, ACTION_PLAY_SEEK);
    pthread_cond_signal(&mPlayer->mHolder->mCondRead);
}

void PacketReader::notifyWait() {
    pthread_cond_signal(&mPlayer->mHolder->mCondRead);
    mPlayer->mAudioDecoder->notifyWait();
    mPlayer->mVideoDecoder->notifyWait();
}
