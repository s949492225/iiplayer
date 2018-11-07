//
// Created by 宋林涛 on 2018/11/5.
//

#include "PacketReader.h"
#include "../MediaPlayer.h"
#include "../decode/Decoder.h"
#include "../render/AudioRender.h"
#include "../render/VideoRender.h"
#include <unistd.h>

#define MAX_QUEUE_SIZE (1024*1024)

PacketReader::PacketReader(MediaPlayer *player) {
    mPlayer = player;
    pthread_mutex_init(&mMutexRead, NULL);
    pthread_cond_init(&mPlayer->getHolder()->mCondRead, NULL);
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

    mPlayer->getHolder()->mFormatCtx = avformat_alloc_context();
    mPlayer->getHolder()->mFormatCtx->interrupt_callback.callback = ioInterruptCallback;
    mPlayer->getHolder()->mFormatCtx->interrupt_callback.opaque = mPlayer;
    const char *url = mPlayer->getUrl();
    int error = avformat_open_input(&mPlayer->getHolder()->mFormatCtx, url, NULL, NULL);
    if (error != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open url %s", url);
        }
        mPlayer->sendMsg(false, ERROR_OPEN_FILE);
        return -1;
    }

    if (avformat_find_stream_info(mPlayer->getHolder()->mFormatCtx, NULL) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not find steams from %s", url);
        }
        mPlayer->sendMsg(false, ERROR_FIND_STREAM);
        return -1;
    }
    //流信息获取
    for (int i = 0; i < mPlayer->getHolder()->mFormatCtx->nb_streams; i++) {
        AVCodecParameters *parameters = mPlayer->getHolder()->mFormatCtx->streams[i]->codecpar;
        if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            //音频解码器
            if (i != -1) {
                int ret = get_codec_context(parameters, &mPlayer->getHolder()->mAudioCodecCtx);
                if (ret < 0) {
                    mPlayer->sendMsg(false, ERROR_AUDIO_DECODEC_EXCEPTION);
                    return -1;
                }
                mPlayer->getHolder()->mAudioStreamIndex = i;
                mPlayer->mDuration = mPlayer->getHolder()->mFormatCtx->duration / AV_TIME_BASE;
                mPlayer->sendMsg(false, DATA_DURATION, static_cast<int>(mPlayer->mDuration));
            }
        } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (i != -1) {
                mPlayer->getHolder()->mVideoCodecParam = parameters;
                int ret = get_codec_context(parameters, &mPlayer->getHolder()->mVideoCodecCtx);
                if (ret < 0) {
                    mPlayer->sendMsg(false, ERROR_VIDEO_DECODEC_EXCEPTION,
                                     static_cast<int>(mPlayer->mDuration));
                    return -1;
                }
                mPlayer->getHolder()->mVideoStreamIndex = i;

            }
        }
    }

    setMediaInfo();

    initBitStreamFilter();

    mPlayer->sendMsg(false, ACTION_PLAY_PREPARED);

    mPlayer->setAudioDecoder(new AudioDecoder(mPlayer));
    mPlayer->getAudioDecoder()->init();
    mPlayer->setAudioRender(new AudioRender(mPlayer));

    if (isHardCodec()) {
        mPlayer->getCallJava()->setCodecType(DECODE_HARD);
        mPlayer->setVideoDecoder(new HardVideoDecoder(mPlayer));
        mPlayer->getVideoDecoder()->init();
    } else {
        mPlayer->getCallJava()->setCodecType(DECODE_SOFT);
        mPlayer->setVideoDecoder(new VideoDecoder(mPlayer));
        mPlayer->getVideoDecoder()->init();
        mPlayer->setVideoRender(new VideoRender(mPlayer));
    }
    return 0;
}

bool PacketReader::isHardCodec() const {
    return !mPlayer->isOnlySoft && mPlayer->getHolder()->mAbsCtx != NULL;
}

void PacketReader::read() {
    if (LOG_DEBUG) {
        LOGD("读取线程启动成功,tid:%i\n", gettid());
    }
    int error_code = prepare();
    if (error_code < 0) {
        return;
    }
    BaseDecoder *audioDecoder = mPlayer->getAudioDecoder();
    BaseDecoder *videoDecoder = mPlayer->getVideoDecoder();
    //read packet
    while (mPlayer->mStatus != NULL && !mPlayer->mStatus->isExit && !mPlayer->mStatus->isPlayEnd) {

        if (mPlayer->mStatus->isEOF && !mPlayer->mStatus->isSeek) {
            pthread_mutex_lock(&mMutexRead);
            thread_wait(&mPlayer->getHolder()->mCondRead, &mMutexRead, 10);
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
                videoDecoder->getQueueSize() +
                audioDecoder->getQueueSize() >
                MAX_QUEUE_SIZE;
        bool isAudioNeed = audioDecoder->getQueueSize() > MAX_QUEUE_SIZE / 2;
        bool isVideoNeed = videoDecoder->getQueueSize() > MAX_QUEUE_SIZE / 2;
        if (isSizeOver & isAudioNeed & isVideoNeed) {
            pthread_mutex_lock(&mMutexRead);
            thread_wait(&mPlayer->getHolder()->mCondRead, &mMutexRead, 10);
            pthread_mutex_unlock(&mMutexRead);
            continue;
        }

        AVPacket *packet = av_packet_alloc();
        int ret;
        if ((ret = av_read_frame(mPlayer->getHolder()->mFormatCtx, packet)) == 0) {

            if (packet->stream_index == mPlayer->getHolder()->mVideoStreamIndex) {

                if (isHardCodec()) {
                    if (mPlayer->getHolder()->mAbsCtx != NULL) {
                        if (av_bsf_send_packet(mPlayer->getHolder()->mAbsCtx, packet) != 0) {
                            av_packet_free(&packet);
                            continue;
                        }
                        int recSuccess = 0;
                        while (recSuccess == 0) {
                            AVPacket *newPacket = av_packet_alloc();
                            recSuccess = av_bsf_receive_packet(mPlayer->getHolder()->mAbsCtx,
                                                               newPacket);
                            videoDecoder->putPacket(newPacket);
                        }
                        av_packet_free(&packet);
                    }
                } else {
                    videoDecoder->putPacket(packet);
                }

            } else if (packet->stream_index == mPlayer->getHolder()->mAudioStreamIndex) {
                audioDecoder->putPacket(packet);
                checkBuffer(packet);
            } else {
                av_packet_free(&packet);
            }

        } else {
            av_packet_free(&packet);
            if ((ret == AVERROR_EOF || avio_feof(mPlayer->getHolder()->mFormatCtx->pb) == 0)) {
                mPlayer->mStatus->isEOF = true;
                continue;
            }
            if (mPlayer->getHolder()->mFormatCtx->pb && mPlayer->getHolder()->mFormatCtx->pb->error) {
                mPlayer->sendMsg(false, ERROR_REDAD_EXCEPTION);
                seekErrorPos(static_cast<int>(mPlayer->mClock));
            }
            pthread_mutex_lock(&mMutexRead);
            thread_wait(&mPlayer->getHolder()->mCondRead, &mMutexRead, 10);
            pthread_mutex_unlock(&mMutexRead);
        }
    }
}

void PacketReader::initBitStreamFilter() {
    const AVBitStreamFilter *filter = NULL;
    const char *codecName = mPlayer->getHolder()->mVideoCodecCtx->codec->name;
    if (mPlayer->getCallJava()->isSupportHard(false, codecName)) {
        if (strcasecmp(codecName, "h264") == 0) {
            filter = av_bsf_get_by_name("h264_mp4toannexb");
        } else if (strcasecmp(codecName, "h265") == 0) {
            filter = av_bsf_get_by_name("hevc_mp4toannexb");
        }
    }

    if (filter == NULL) {
        return;
    }

    if (av_bsf_alloc(filter, &mPlayer->getHolder()->mAbsCtx) != 0) {
        return;
    }
    if (avcodec_parameters_copy(mPlayer->getHolder()->mAbsCtx->par_in,
                                mPlayer->getHolder()->mVideoCodecParam) < 0) {
        av_bsf_free(&mPlayer->getHolder()->mAbsCtx);
        mPlayer->getHolder()->mAbsCtx = NULL;
        return;
    }
    if (av_bsf_init(mPlayer->getHolder()->mAbsCtx) != 0) {
        av_bsf_free(&mPlayer->getHolder()->mAbsCtx);
        mPlayer->getHolder()->mAbsCtx = NULL;
        return;
    }
    mPlayer->getHolder()->mAbsCtx->time_base_in = mPlayer->getHolder()->mFormatCtx->streams[mPlayer->getHolder()->mVideoStreamIndex]->time_base;

}


void PacketReader::handlerSeek() {
    //seek io
    int64_t rel = mPlayer->mStatus->mSeekSec * AV_TIME_BASE;
    int ret = avformat_seek_file(mPlayer->getHolder()->mFormatCtx, -1, INT64_MIN, rel, INT64_MAX,
                                 AVSEEK_FLAG_BACKWARD);
    if (ret == 0) {
        //clear
        mPlayer->getAudioDecoder()->clearQueue();
        mPlayer->getVideoDecoder()->clearQueue();

        mPlayer->getAudioRender()->clearQueue();
        if (mPlayer->getVideoRender()) {
            mPlayer->getVideoRender()->clearQueue();
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
    double cachedTime = packet->pts * av_q2d(mPlayer->getAudioRender()->mTimebase);
    mPlayer->getCallJava()->sendMsg(false, DATA_BUFFER_TIME, static_cast<int>(cachedTime));
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
    pthread_cond_signal(&mPlayer->getHolder()->mCondRead);
}

void PacketReader::setMediaInfo() {
    AVDictionaryEntry *tag = NULL;
    tag = av_dict_get(
            mPlayer->getHolder()->mFormatCtx->streams[mPlayer->getHolder()->mVideoStreamIndex]->metadata,
            "rotate", tag, 0);
    if (tag == NULL) {
        mPlayer->mRotation = 0;
    } else {
        int angle = atoi(tag->value);
        angle %= 360;
        mPlayer->mRotation = angle;
    }

    mPlayer->mWidth = mPlayer->getHolder()->mVideoCodecCtx->width;
    mPlayer->mHeight = mPlayer->getHolder()->mVideoCodecCtx->height;
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
    pthread_cond_signal(&mPlayer->getHolder()->mCondRead);
}

void PacketReader::notifyWait() {
    pthread_cond_signal(&mPlayer->getHolder()->mCondRead);
    mPlayer->getAudioDecoder()->notifyWait();
    mPlayer->getVideoDecoder()->notifyWait();
}
