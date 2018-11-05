//
// Created by 宋林涛 on 2018/10/30.
//
#include "VideoRender.h"
#import "MediaPlayer.h"

VideoRender::VideoRender(MediaPlayer *player, AVCodecContext *codecContext, AVRational timebase) {
    mPlayer = player;
    mStatus = player->mStatus;
    mPixFmt = codecContext->pix_fmt;
    mTimebase = timebase;
    mQueue = new FrameQueue(mStatus, const_cast<char *>("video"));
    mWidth = codecContext->width;
    mHeight = codecContext->height;

}

void VideoRender::play() {
    mPlayThread = new std::thread(std::bind(&VideoRender::playThread, this));
}

void VideoRender::playThread() {
    LOGD("视频播放线程开始,tid:%i\n", gettid());
    while (mStatus != NULL && !mStatus->isExit) {

        if (mStatus->isSeek) {
            av_usleep(1000 * 5);
            continue;
        }

        if (mStatus->isPause) {
            av_usleep(1000 * 5);
            continue;
        }

        if (mQueue->getQueueSize() == 0)//加载中
        {
            av_usleep(1000 * 10);
            continue;
        }

        AVFrame *frame = av_frame_alloc();
        int ret = mQueue->getFrame(frame);

        if (ret == 0) {
            if (mStatus && mStatus->isSeek) {
                double frame_time = frame->pts * av_q2d(mTimebase);
                if (fabs(mStatus->mSeekSec - frame_time) > 0.01) {
                    av_frame_free(&frame);
                    continue;
                }
            }

            if (frame->format == AV_PIX_FMT_YUV420P) {
                double diff = getFrameDiffTime(frame);
                if (diff < -0.01) {
                    int sleep = -(int) (diff * 1000);
                    if (fabs(sleep) > 50) {
                        sleep = 50;
                    }
                    av_usleep(static_cast<unsigned int>(sleep * 1000));
                    if (mStatus == NULL || mStatus->isExit) {
                        av_frame_free(&frame);
                        break;
                    }
                } else if (diff > 0.05) {
                    av_frame_free(&frame);
                    continue;
                }
                mPlayer->getCallJava()->setFrameData(false, mWidth, mHeight,
                                                     frame->data[0],
                                                     frame->data[1],
                                                     frame->data[2]);
            } else {
                LOGE("当前视频不是YUV420P格式");
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
                    continue;
                }
                sws_scale(
                        sws_ctx,
                        reinterpret_cast<const uint8_t *const *>(frame->data),
                        frame->linesize,
                        0,
                        frame->height,
                        yuvFrame->data,
                        yuvFrame->linesize);


                double diff = getFrameDiffTime(frame);

                if (diff < -0.01) {
                    int sleep = -(int) (diff * 1000);
                    if (fabs(sleep) > 50) {
                        sleep = 50;
                    }
                    av_usleep(static_cast<unsigned int>(sleep * 1000));
                    if (mStatus == NULL || mStatus->isExit) {
                        av_frame_free(&yuvFrame);
                        av_frame_free(&frame);
                        av_free(buffer);
                        break;
                    }
                }

                mPlayer->getCallJava()->setFrameData(false, mWidth, mHeight,
                                                     yuvFrame->data[0],
                                                     yuvFrame->data[1],
                                                     yuvFrame->data[2]);
                sws_freeContext(sws_ctx);
                av_frame_free(&yuvFrame);
                av_free(buffer);

            }
        }
        av_frame_free(&frame);

    }

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

    double diff = mPlayer->mClock - pts;
    return diff;
}

void VideoRender::putFrame(AVFrame *frame) {
    mQueue->putFrame(frame);

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
