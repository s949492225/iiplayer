//
// Created by 宋林涛 on 2018/10/30.
//
#include "VideoRender.h"
#import "../MediaPlayer.h"
#include <cstdlib>

VideoRender::VideoRender(MediaPlayer *player) {
    mPlayer = player;
    mPixFmt = mPlayer->getHolder()->mVideoCodecCtx->pix_fmt;
    mWidth = mPlayer->getWidth();
    mHeight = mPlayer->getHeight();
    mTimebase = mPlayer->getHolder()->getVideoTimeBase();
    mQueue = new FrameQueue(mPlayer->getStatus(), const_cast<char *>("video"));

}

void VideoRender::play() {
    mPlayThread = new std::thread(std::bind(&VideoRender::playThread, this));
}

void VideoRender::playThread() {
    LOGD("视频播放线程开始,tid:%i\n", gettid());
    while (mPlayer->getStatus() != NULL && !mPlayer->getStatus()->isExit) {
        AVFrame *frame = mQueue->getFrame();
        if (frame != NULL) {

            AVFrame *yuvFrame = NULL;
            if (frame->format == AV_PIX_FMT_YUV420P) {
                yuvFrame = frame;
            } else {
                yuvFrame = scale(frame);
                if (yuvFrame == NULL)
                    continue;
            }

            double diff = getFrameDiffTime(yuvFrame);

            if (!mPlayer->getStatus()->isPause && diff < -0.01) {
                int sleep = -(int) (diff * 1000);
                if (std::abs(sleep) > 50) {
                    sleep = 50;
                }
                av_usleep(static_cast<unsigned int>(sleep * 1000));
                if (mPlayer->getStatus() == NULL || mPlayer->getStatus()->isExit) {
                    av_frame_free(&yuvFrame);
                    break;
                }
            }

            mPlayer->getCallJava()->setFrameData(false, mWidth, mHeight,
                                                 frame->data[0],
                                                 frame->data[1],
                                                 frame->data[2]);
        }
    }

}

AVFrame *VideoRender::scale(AVFrame *frame) {
    AVFrame *yuvFrame = av_frame_alloc();
    int num = av_image_get_buffer_size(
            AV_PIX_FMT_YUV420P,
            mWidth,
            mHeight,
            1);
    uint8_t *buffer = static_cast<uint8_t *>(av_malloc(num * sizeof(uint8_t)));

    av_image_fill_arrays(
            yuvFrame->data,
            yuvFrame->linesize,
            buffer,
            AV_PIX_FMT_YUV420P,
            mWidth,
            mHeight,
            1);
    SwsContext *sws_ctx = sws_getContext(
            mWidth,
            mHeight,
            mPixFmt,
            mWidth,
            mHeight,
            AV_PIX_FMT_YUV420P,
            SWS_BICUBIC, NULL, NULL, NULL);

    if (!sws_ctx) {
        av_frame_free(&yuvFrame);
        av_free(yuvFrame);
        av_free(buffer);
        return NULL;
    }
    sws_scale(
            sws_ctx,
            reinterpret_cast<const uint8_t *const *>(frame->data),
            frame->linesize,
            0,
            frame->height,
            yuvFrame->data,
            yuvFrame->linesize);
    return yuvFrame;
}


double VideoRender::getFrameDiffTime(AVFrame *frame) {
    double pts = av_frame_get_best_effort_timestamp(frame);
    if (pts == AV_NOPTS_VALUE) {
        pts = 0;
    }
    pts *= av_q2d(mTimebase);

    if (pts == 0) {
        return 0;
    }

    double diff = mPlayer->getClock() - pts;
    return diff;
}

void VideoRender::putFrame(AVFrame *frame) {
    if (mQueue) {
        mQueue->putFrame(frame);
    }

}

bool VideoRender::isQueueFull() {
    return mQueue->getQueueSize() >= mMaxQueueSize;
}

void VideoRender::clearQueue() {
    mQueue->clearAll();
}


void VideoRender::notifyWait() {
    mQueue->notifyAll();
}

VideoRender::~VideoRender() {
    if (mPlayThread != NULL) {
        mPlayThread->join();
        mPlayThread = NULL;
    }

    if (mQueue != NULL) {
        delete mQueue;
        mQueue = NULL;
    }
}
