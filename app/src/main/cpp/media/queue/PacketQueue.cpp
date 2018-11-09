//
// Created by 宋林涛 on 2018/9/28.
//

#include "PacketQueue.h"
#include "../global/Status.h"

PacketQueue::PacketQueue(Status *status, LifeSequenceHolder *holder, char *name) {
    mStatus = status;
    mName = name;
    mHolder = holder;
    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCond, NULL);
}

PacketQueue::~PacketQueue() {
    mHolder = NULL;
    clearAll();
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCond);
}

int PacketQueue::putPacket(AVPacket *packet) {
    pthread_mutex_lock(&mMutex);

    mQueue.push(packet);
    pthread_cond_broadcast(&mCond);

    pthread_mutex_unlock(&mMutex);
    return 0;
}

AVPacket *PacketQueue::getPacket() {
    pthread_mutex_lock(&mMutex);
    AVPacket *ret = NULL;
    while (mStatus != NULL && !mStatus->isExit) {
        if (mQueue.size() > 0) {
            ret = mQueue.front();
            mQueue.pop();
            break;
        } else {
            pthread_cond_broadcast(&mHolder->mReadCond);
            pthread_cond_wait(&mCond, &mMutex);
        }
    }
    pthread_mutex_unlock(&mMutex);
    return ret;
}

int PacketQueue::getQueueSize() {
    int size = 0;
    pthread_mutex_lock(&mMutex);
    size = mQueue.size();
    pthread_mutex_unlock(&mMutex);
    return size;
}

void PacketQueue::clearAll() {
    pthread_cond_broadcast(&mCond);
    pthread_mutex_lock(&mMutex);

    while (!mQueue.empty()) {
        AVPacket *packet = mQueue.front();
        mQueue.pop();
        av_packet_free(&packet);
        av_free(packet);
        packet = NULL;
    }
    pthread_mutex_unlock(&mMutex);
}

void PacketQueue::notifyAll() {
    pthread_cond_broadcast(&mCond);
}

