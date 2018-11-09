//
// Created by 宋林涛 on 2018/9/28.
//

#ifndef VIDEODEMO_SQUEUE_H
#define VIDEODEMO_SQUEUE_H

#include "queue"
#include "pthread.h"
#include "../../android/android_log.h"
#include "../global/LifeSequenceHolder.h"

extern "C"
{
#include "libavcodec/avcodec.h"
};

class Status;

class Packet {
public:
    AVPacket *pkt = NULL;
    bool isSplit = false;

    Packet();
    Packet(bool isSplit);

    ~Packet();

};

class PacketQueue {
private:
    char *mName;
    std::queue<Packet *> mQueue;
    pthread_mutex_t mMutex;
    pthread_cond_t mCond;
    LifeSequenceHolder *mHolder;
    Status *mStatus = NULL;
public:
    PacketQueue(Status *status, LifeSequenceHolder *holder, char *name);

    ~PacketQueue();

    int putPacket(Packet *packet);

    Packet *getPacket();

    int getQueueSize();

    void clearAll();

    void notifyAll();

};

#endif //VIDEODEMO_SQUEUE_H
