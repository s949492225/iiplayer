//
// Created by 宋林涛 on 2018/11/6.
//

#include "Decoder.h"
#include "../MediaPlayer.h"

HardVideoDecoder::HardVideoDecoder(MediaPlayer *player) : BaseDecoder(player) {

}

void HardVideoDecoder::init() {
    start();
}

void HardVideoDecoder::decode() {
    if (LOG_DEBUG) {
        LOGD("视频[硬]解码线程开始,tid:%i\n", gettid())
    }
    JNIEnv *jniEnv;
    get_jni_jvm()->AttachCurrentThread(&jniEnv, 0);
    jobject surface = mPlayer->getVideoRender()->getMediaCodecSurface(jniEnv);
    if (surface == NULL) {
        return;
    }

    int code = mPlayer->getCallJava()->initMediaCodec(jniEnv,
                                                      surface,
                                                      const_cast<char *>(mPlayer->getHolder()->mVideoCodecCtx->codec->name),
                                                      mPlayer->getWidth(),
                                                      mPlayer->getHeight(),
                                                      mPlayer->getHolder()->mVideoCodecCtx->extradata_size,
                                                      mPlayer->getHolder()->mVideoCodecCtx->extradata_size,
                                                      mPlayer->getHolder()->mVideoCodecCtx->extradata,
                                                      mPlayer->getHolder()->mVideoCodecCtx->extradata);
    if (code != 0) {
        if (code == -1) {
            mPlayer->sendMsg(false, ERROR_OPEN_HARD_CODEC);
        } else if (code == -2) {
            mPlayer->sendMsg(false, ERROR_SURFACE_NULL);
        } else if (code == -3) {
            mPlayer->sendMsg(false, ERROR_JNI);
        }

        return;
    }

    while (mPlayer->getStatus() != NULL && !mPlayer->getStatus()->isExit) {
        if (mPlayer->getStatus()->isSeek) {
            av_usleep(1000 * 10);
            continue;
        }

        Packet *packet = mQueue->getPacket();
        if (packet == NULL) {
            continue;
        }

        if (packet->isSplit) {
            avcodec_flush_buffers(mPlayer->getHolder()->mVideoCodecCtx);
            ii_deletep(&packet);
            mPlayer->getStatus()->mStep = 1;
            continue;
        }

        if (mPlayer->getStatus()->isPause && mPlayer->getStatus()->mStep == 0) {
            av_usleep(1000 * 10);
            continue;
        }

        double diff = getPacketDiffTime(packet->pkt);
        if (!mPlayer->getStatus()->isPause && diff < -0.01) {
            int sleep = -(int) (diff * 1000);
            if (std::abs(sleep) > 50) {
                sleep = 50;
            }
            av_usleep(static_cast<unsigned int>(sleep * 1000));
            if (mPlayer->getStatus() == NULL || mPlayer->getStatus()->isExit) {
                ii_deletep(&packet);
                break;
            }
        }

        if (mPlayer->getStatus()->mStep > 0) {
            mPlayer->getStatus()->mStep--;
        }

        mPlayer->getCallJava()->decodeAVPacket(jniEnv, packet->pkt->size, packet->pkt->data);
        mPlayer->getVideoRender()->onTextureReady();

        ii_deletep(&packet);
    }

    jniEnv->DeleteLocalRef(surface);
    get_jni_jvm()->DetachCurrentThread();

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

    double diff = mPlayer->getClock() - pts;
    return diff;
}


