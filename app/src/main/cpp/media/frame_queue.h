//
// Created by 宋林涛 on 2018/9/28.
//

#pragma once

#include "queue"
#include "pthread.h"
#include "../android/android_log.h"

extern "C"
{
#include "libavcodec/avcodec.h"
};

class Status;

class frame_queue {
public:
    std::queue<AVFrame *> queuePacket;
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;
    Status *playStatus=NULL;
public:
    frame_queue(Status *playStatus);

    ~frame_queue();

    int putFrame(AVFrame *packet);

    int getFrame(AVFrame* frame);

    int getQueueSize();

    void clearAll();

};

