//
// Created by 宋林涛 on 2018/9/28.
//

#include "PacketQueue.h"
#include "Status.h"

PacketQueue::PacketQueue(Status *status) {
    this->mStatus = status;
    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCond, NULL);
}

PacketQueue::~PacketQueue() {
    clearAll();
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCond);
}

int PacketQueue::putPacket(AVPacket *packet) {
    pthread_mutex_lock(&mMutex);

    mQueue.push(packet);
    pthread_cond_signal(&mCond);

    pthread_mutex_unlock(&mMutex);
    return 0;
}

int PacketQueue::getPacket(AVPacket *packet) {
    pthread_mutex_lock(&mMutex);
    int ret = -1;
    while (mStatus != NULL && !mStatus->isExit) {
        if (mQueue.size() > 0) {
            AVPacket *avPacket = mQueue.front();
            if (av_packet_ref(packet, avPacket) == 0) {
                mQueue.pop();
                ret = 0;
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                pthread_cond_signal(&mCond);
            }
            break;
        } else {
            pthread_cond_signal(&mStatus->mCondRead);
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
    pthread_cond_signal(&mCond);
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
    pthread_cond_signal(&mCond);
}

