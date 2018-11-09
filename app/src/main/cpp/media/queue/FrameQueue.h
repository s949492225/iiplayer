//
// Created by 宋林涛 on 2018/9/28.
//

#pragma once

#include "queue"
#include "pthread.h"
#include "../../android/android_log.h"

extern "C"
{
#include "libavcodec/avcodec.h"
};

class Status;

class FrameQueue {
private:
    char *mName;
    std::queue<AVFrame *> mQueue;
    pthread_mutex_t mMutex;
    pthread_cond_t mCond;
    Status *mStatus = NULL;
public:
    FrameQueue(Status *status, char *name);

    ~FrameQueue();

    int putFrame(AVFrame *packet);

    AVFrame *getFrame();

    int getQueueSize();

    void clearAll();

    void notifyAll();

};

