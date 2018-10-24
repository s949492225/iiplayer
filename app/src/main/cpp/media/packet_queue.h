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

class status;

class packet_queue {
public:
    std::queue<AVPacket *> queuePacket;
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;
    status *playStatus;
public:
    packet_queue(status *playStatus);

    ~packet_queue();

    int putPacket(AVPacket *packet);

    int getPacket(AVPacket *packet);

    int getQueueSize();

    void clearAll();

};

#endif //VIDEODEMO_SQUEUE_H
