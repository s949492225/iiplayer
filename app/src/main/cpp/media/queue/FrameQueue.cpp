//
// Created by 宋林涛 on 2018/9/28.
//

#include "FrameQueue.h"
#include "../global/Status.h"


FrameQueue::FrameQueue(Status *status, char *name) {
    mStatus = status;
    mName = name;
    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCond, NULL);
}

FrameQueue::~FrameQueue() {
    clearAll();
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCond);
}

int FrameQueue::putFrame(AVFrame *packet) {
    pthread_mutex_lock(&mMutex);
    mQueue.push(packet);
    pthread_cond_broadcast(&mCond);
    pthread_mutex_unlock(&mMutex);
    return 0;
}

AVFrame *FrameQueue::getFrame() {
    pthread_mutex_lock(&mMutex);
    AVFrame *ret = NULL;
    while (mStatus != NULL && !mStatus->isExit) {
        if (mQueue.size() > 0) {
            ret = mQueue.front();
            mQueue.pop();
            pthread_cond_broadcast(&mCond);
            break;
        } else {
            pthread_cond_wait(&mCond, &mMutex);
        }
    }

    pthread_mutex_unlock(&mMutex);
    return ret;
}

int FrameQueue::getQueueSize() {
    int size = 0;
    pthread_mutex_lock(&mMutex);
    size = mQueue.size();
    pthread_mutex_unlock(&mMutex);
    return size;
}

void FrameQueue::clearAll() {
    pthread_cond_broadcast(&mCond);
    pthread_mutex_lock(&mMutex);

    while (!mQueue.empty()) {
        AVFrame *packet = mQueue.front();
        mQueue.pop();
        av_frame_free(&packet);
        av_free(packet);
        packet = NULL;
    }
    pthread_mutex_unlock(&mMutex);
}

void FrameQueue::notifyAll() {
    pthread_cond_broadcast(&mCond);
}

