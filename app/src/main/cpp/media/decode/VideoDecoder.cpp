//
// Created by 宋林涛 on 2018/11/5.
//

#include "Decoder.h"
#include "../MediaPlayer.h"

VideoDecoder::VideoDecoder(MediaPlayer *player) : BaseDecoder(player) {
}

void VideoDecoder::init() {
    start();
}

void VideoDecoder::decode() {
    if (LOG_DEBUG) {
        LOGD("视频解码线程开始,tid:%i\n", gettid())
    }
    AVPacket *packet = NULL;
    AVFrame *frame = NULL;
    int ret = 0;

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

        ret = avcodec_send_packet(mPlayer->getHolder()->mVideoCodecCtx, packet);
        if (ret != 0) {
            av_packet_free(&packet);
            continue;
        }

        frame = av_frame_alloc();
        ret = avcodec_receive_frame(mPlayer->getHolder()->mVideoCodecCtx, frame);

        if (ret == 0) {

            while (mPlayer->mStatus != NULL && !mPlayer->mStatus->isExit &&
                   mPlayer->getVideoRender()->isQueueFull()) {
                av_usleep(1000 * 5);
            }
            if (mPlayer->mStatus == NULL || mPlayer->mStatus->isExit) {
                av_frame_free(&frame);
                continue;
            }
            if (mPlayer->getVideoRender() && !mPlayer->mStatus->isSeek) {
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
