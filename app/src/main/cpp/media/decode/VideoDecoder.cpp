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

    while (mPlayer->getStatus() != NULL && !mPlayer->getStatus()->isExit) {
        if (mPlayer->getStatus()->isSeek) {
            av_usleep(1000 * 100);
            continue;
        }

        if (mPlayer->getStatus()->isPause) {
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

            while (mPlayer->getStatus() != NULL && !mPlayer->getStatus()->isExit &&
                   mPlayer->getVideoRender()->isQueueFull()) {
                av_usleep(1000 * 5);
            }
            if (mPlayer->getStatus() == NULL || mPlayer->getStatus()->isExit) {
                av_frame_free(&frame);
                continue;
            }
            if (mPlayer->getVideoRender() && !mPlayer->getStatus()->isSeek) {
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
