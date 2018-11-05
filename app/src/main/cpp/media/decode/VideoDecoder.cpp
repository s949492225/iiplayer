//
// Created by 宋林涛 on 2018/11/5.
//

#include "VideoDecoder.h"
#include "../MediaPlayer.h"

VideoDecoder::VideoDecoder(MediaPlayer *player) {
    mPlayer = player;
    mQueue = new PacketQueue(mPlayer->mStatus, mPlayer->mHolder, const_cast<char *>("video"));
}

VideoDecoder::~VideoDecoder() {
    mDecodeThread->join();
    mDecodeThread = NULL;
    mPlayer = NULL;
}

void VideoDecoder::start() {
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
//        long time0 = getCurrentTime();
        
        ret = avcodec_send_packet(mPlayer->mHolder->mVideoCodecCtx, packet);
        if (ret != 0) {
            av_packet_free(&packet);
            continue;
        }

        frame = av_frame_alloc();
        ret = avcodec_receive_frame(mPlayer->mHolder->mVideoCodecCtx, frame);
//        long time1 = getCurrentTime();
//        LOGD("解码的时长:%ld", time1 - time0);
        if (ret == 0) {

            while (mPlayer->mStatus != NULL && !mPlayer->mStatus->isExit &&
                   mPlayer->getVideoRender()->isQueueFull()) {
                av_usleep(1000 * 5);
            }
            if (mPlayer->mStatus == NULL || mPlayer->mStatus->isExit) {
                av_frame_free(&frame);
                continue;
            }
            if (mPlayer->getVideoRender()&&!mPlayer->mStatus->isSeek) {
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