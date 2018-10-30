//
// Created by 宋林涛 on 2018/9/28.
//

#ifndef VIDEODEMO_SQUEUE_H
#define VIDEODEMO_SQUEUE_H

#include "queue"
#include "pthread.h"
#include "../android/android_log.h"

extern "C"
{
#include "libavcodec/avcodec.h"
};

class Status;

class PacketQueue {
private:
    std::queue<AVPacket *> mQueue;
    pthread_mutex_t mMutex;
    pthread_cond_t mCond;
    Status *mStatus = NULL;
public:
    PacketQueue(Status *status);

    ~PacketQueue();

    int putPacket(AVPacket *packet);

    int getPacket(AVPacket *packet);

    int getQueueSize();

    void clearAll();

    void notifyAll();

};

#endif //VIDEODEMO_SQUEUE_H
