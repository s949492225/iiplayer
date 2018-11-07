//
// Created by 宋林涛 on 2018/11/6.
//

#include "Decoder.h"
#include "../MediaPlayer.h"

HardVideoDecoder::HardVideoDecoder(MediaPlayer *player) : BaseDecoder(player) {

}

void HardVideoDecoder::init() {
    int code = mPlayer->getCallJava()->initMediaCodec(false,
                                                  const_cast<char *>(mPlayer->getHolder()->mVideoCodecCtx->codec->name),
                                                  mPlayer->mWidth,
                                                  mPlayer->mHeight,
                                                  mPlayer->getHolder()->mVideoCodecCtx->extradata_size,
                                                  mPlayer->getHolder()->mVideoCodecCtx->extradata_size,
                                                  mPlayer->getHolder()->mVideoCodecCtx->extradata,
                                                  mPlayer->getHolder()->mVideoCodecCtx->extradata);
    if (code == 0) {
        start();
    } else {
        if (code == -1) {
            mPlayer->sendMsg(false, ERROR_OPEN_HARD_CODEC);
        } else if (code == -2) {
            mPlayer->sendMsg(false, ERROR_SURFACE_NULL);
        } else if (code == -3) {
            mPlayer->sendMsg(false, ERROR_JNI);
        }
    }
}

void HardVideoDecoder::decode() {
    if (LOG_DEBUG) {
        LOGD("视频解码线程开始,tid:%i\n", gettid())
    }
    AVPacket *packet = NULL;

    while (mPlayer->mStatus != NULL && !mPlayer->mStatus->isExit) {
        if (mPlayer->mStatus->isSeek) {
            av_usleep(1000 * 100);
            continue;
        }

        if (mPlayer->mStatus->isPause) {
            av_usleep(1000 * 100);
            continue;
        }

        packet = av_packet_alloc();
        if (mQueue->getPacket(packet) != 0) {
            av_packet_free(&packet);
            continue;
        }

        double diff = getPacketDiffTime(packet);
        if (diff < -0.01) {
            int sleep = -(int) (diff * 1000);
            if (std::abs(sleep) > 50) {
                sleep = 50;
            }
            av_usleep(static_cast<unsigned int>(sleep * 1000));
            if (mPlayer->mStatus == NULL || mPlayer->mStatus->isExit) {
                av_packet_free(&packet);
                break;
            }
        }

        mPlayer->getCallJava()->decodeAVPacket(false, packet->size, packet->data);

        av_packet_free(&packet);

    }
}

double HardVideoDecoder::getPacketDiffTime(AVPacket *packet) {
    double pts = packet->pts;
    if (pts == AV_NOPTS_VALUE) {
        pts = 0;
    }
    pts *= av_q2d(
            mPlayer->getHolder()->mFormatCtx->streams[mPlayer->getHolder()->mVideoStreamIndex]->time_base);

    if (pts == 0) {
        return 0;
    }

    double diff = mPlayer->mClock - pts;
    return diff;
}


