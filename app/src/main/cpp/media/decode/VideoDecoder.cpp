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
    int ret = 0;
    while (mPlayer->getStatus() != NULL && !mPlayer->getStatus()->isExit) {
        if (mPlayer->getStatus()->isSeek) {
            av_usleep(1000 * 10);
            continue;
        }

        if (mPlayer->getStatus()->isPause) {
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
            continue;
        }

        ret = avcodec_send_packet(mPlayer->getHolder()->mVideoCodecCtx, packet->pkt);
        if (ret != 0) {
            ii_deletep(&packet);
            continue;
        }

        while (ret >= 0) {
            AVFrame *frame = av_frame_alloc();
            ret = avcodec_receive_frame(mPlayer->getHolder()->mVideoCodecCtx, frame);
            if (ret >= 0) {

                while (mPlayer->getStatus() != NULL && !mPlayer->getStatus()->isExit &&
                       mPlayer->getVideoRender()->isQueueFull()) {
                    av_usleep(1000 * 5);
                }
                if (mPlayer->getStatus() == NULL || mPlayer->getStatus()->isExit) {
                    av_frame_free(&frame);
                    break;
                }
                mPlayer->getVideoRender()->putFrame(frame);

            } else {
                av_frame_free(&frame);
            }
        }
        ii_deletep(&packet);
    }
}
