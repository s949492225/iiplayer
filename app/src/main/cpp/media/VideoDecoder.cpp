//
// Created by 宋林涛 on 2018/11/5.
//

#include "VideoDecoder.h"
#include "MediaPlayer.h"

VideoDecoder::VideoDecoder(MediaPlayer *player) {
    mPlayer = player;
    mStatus = player->mStatus;
}

VideoDecoder::~VideoDecoder() {
    mDecodeThread->join();
    mDecodeThread = NULL;
    mPlayer = NULL;
    mStatus = NULL;
    mCoderCtx = NULL;
}

void VideoDecoder::start(AVCodecContext *pContext) {
    mCoderCtx = pContext;
    mDecodeThread = new std::thread(std::bind(&VideoDecoder::decode, this));
}

//long getCurrentTime() {
//    struct timeval tv;
//    gettimeofday(&tv, NULL);
//    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
//}

void VideoDecoder::decode() {
    if (LOG_DEBUG) {
        LOGD("视频解码线程开始,tid:%i\n", gettid())
    }
    AVPacket *packet = NULL;
    AVFrame *frame = NULL;
    int ret = 0;

    while (mStatus != NULL && !mStatus->isExit) {
        if (mStatus->isSeek) {
            av_usleep(1000 * 100);
            continue;
        }

        if (mStatus->isPause) {
            av_usleep(1000 * 100);
            continue;
        }

        packet = av_packet_alloc();
        if (mStatus->mVideoQueue->getPacket(packet) != 0) {
            av_packet_free(&packet);
            continue;
        }
//        long time0 = getCurrentTime();
        ret = avcodec_send_packet(mCoderCtx, packet);
        if (ret != 0) {
            av_packet_free(&packet);
            continue;
        }

        frame = av_frame_alloc();
        ret = avcodec_receive_frame(mCoderCtx, frame);
//        long time1 = getCurrentTime();
//        LOGD("解码的时长:%ld", time1 - time0);
        if (ret == 0) {

            while (mStatus != NULL && !mStatus->isExit && mPlayer->getVideoRender()->isQueueFull()) {
                av_usleep(1000 * 5);
            }
            if (mStatus == NULL || mStatus->isExit) {
                av_frame_free(&frame);
                continue;
            }
            if (!mStatus->isSeek) {
                mPlayer->getVideoRender()->putFrame(frame);
            } else {
                av_frame_free(&frame);
            }
            av_packet_free(&packet);
        } else {
            av_frame_free(&frame);
            av_packet_free(&packet);
        }
    }
}