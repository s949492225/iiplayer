//
// Created by 宋林涛 on 2018/10/30.
//
#include "VideoRender.h"
#import "MediaPlayer.h"

VideoRender::VideoRender(MediaPlayer *player, AVCodecContext *codecContext, void *render) {
    mPlayer = player;
    mStatus = player->mStatus;
    mPixFmt = codecContext->pix_fmt;
    mTimebase = codecContext->time_base;
    mQueue = new FrameQueue(mStatus);
    mGLRender = render;
    mWidth = codecContext->width;
    mHeight = codecContext->height;

}

void VideoRender::play() {
    mPlayThread = new std::thread(std::bind(&VideoRender::playThread, this));
}

void VideoRender::playThread() {
    LOGD("视频播放线程开始,tid:%i\n", gettid());
    while (mStatus != NULL && !mStatus->isExit) {

        if (mQueue->getQueueSize() == 0)//加载中
        {
            if (!mStatus->isLoad) {
                mStatus->isLoad = true;
                mPlayer->sendMsg(ACTION_PLAY_LOADING);
            }
            av_usleep(1000 * 100);
            continue;
        } else {
            if (mStatus->isLoad) {
                mStatus->isLoad = false;
                mPlayer->sendMsg(ACTION_PLAY_LOADING_OVER);
            }
        }

        AVFrame *frame = av_frame_alloc();
        int ret = mQueue->getFrame(frame);
        if (ret == 0) {
            double diff = getFrameDiffTime(frame);
            av_usleep((unsigned int) getDelayTime(diff) * 1000000);
            if (mStatus == NULL || mStatus->isExit) {
                av_frame_free(&frame);
                break;
            }

            if (frame->format == AV_PIX_FMT_YUV420P) {
                renderFrame(frame);
            } else {
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
                        mWidth,
                        1);

                SwsContext *swsCtx = sws_getContext(
                        mWidth,
                        mHeight,
                        mPixFmt,
                        mWidth,
                        mHeight,
                        AV_PIX_FMT_YUV420P,
                        SWS_BICUBIC, NULL, NULL, NULL);

                if (!swsCtx) {
                    av_frame_free(&frame);
                    av_frame_free(&yuvFrame);
                    av_free(buffer);
                    continue;
                }

                sws_scale(
                        swsCtx,
                        (const uint8_t *const *) (frame->data),
                        frame->linesize,
                        0,
                        frame->height,
                        yuvFrame->data,
                        yuvFrame->linesize);

                renderFrame(yuvFrame);

                av_frame_free(&yuvFrame);
                av_free(buffer);
                sws_freeContext(swsCtx);
            }
        }
        av_frame_free(&frame);

    }

}

void VideoRender::renderFrame(const AVFrame *yuvFrame) const {
    JNIEnv *env = get_jni_env();


    jbyteArray y = env->NewByteArray(mWidth * mHeight);
    env->SetByteArrayRegion(y, 0, mWidth * mHeight,
                            reinterpret_cast<const jbyte *>(yuvFrame->data[0]));

    jbyteArray u = env->NewByteArray(mWidth * mHeight / 4);
    env->SetByteArrayRegion(u, 0, mWidth * mHeight / 4,
                            reinterpret_cast<const jbyte *>(yuvFrame->data[1]));

    jbyteArray v = env->NewByteArray(mWidth * mHeight / 4);
    env->SetByteArrayRegion(v, 0, mWidth * mHeight / 4,
                            reinterpret_cast<const jbyte *>(yuvFrame->data[2]));


    jclass jcs = env->GetObjectClass(static_cast<jobject>(mGLRender));
    jmethodID jmid = env->GetMethodID(jcs, "setYUVRenderData", "(II[B[B[B)V");
    env->CallVoidMethod(static_cast<jobject>(mGLRender), jmid, mWidth, mHeight, y, u, v);

    env->DeleteLocalRef(y);
    env->DeleteLocalRef(u);
    env->DeleteLocalRef(v);
}

double VideoRender::getFrameDiffTime(AVFrame *avFrame) {

    double pts = av_frame_get_best_effort_timestamp(avFrame);
    if (pts == AV_NOPTS_VALUE) {
        pts = 0;
    }
    pts *= av_q2d(mTimebase);

    if (pts > 0) {
        mNowTime = pts;
    }

    double diff = mPlayer->mPlayTime - mNowTime;
    return diff;
}

double VideoRender::getDelayTime(double diff) {

    if (diff > 0.003) {
        delayTime = delayTime * 2 / 3;
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    } else if (diff < -0.003) {
        delayTime = delayTime * 3 / 2;
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    } else if (diff == 0.003) {

    }
    if (diff >= 0.5) {
        delayTime = 0;
    } else if (diff <= -0.5) {
        delayTime = defaultDelayTime * 2;
    }

    if (fabs(diff) >= 10) {
        delayTime = defaultDelayTime;
    }
    return delayTime;
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
