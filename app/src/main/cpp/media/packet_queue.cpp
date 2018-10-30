//
// Created by 宋林涛 on 2018/9/28.
//

#include "packet_queue.h"
#include "Status.h"

packet_queue::packet_queue(Status *playStatus) {
    this->playStatus = playStatus;
    pthread_mutex_init(&mutexPacket, NULL);
    pthread_cond_init(&condPacket, NULL);
}

packet_queue::~packet_queue() {
    clearAll();
    pthread_mutex_destroy(&mutexPacket);
    pthread_cond_destroy(&condPacket);
}

int packet_queue::putPacket(AVPacket *packet) {
    pthread_mutex_lock(&mutexPacket);

    queuePacket.push(packet);
    pthread_cond_signal(&condPacket);

    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int packet_queue::getPacket(AVPacket *packet) {
    pthread_mutex_lock(&mutexPacket);
    int ret = -1;
    while (playStatus != NULL && !playStatus->mExit) {
        if (queuePacket.size() > 0) {
            AVPacket *avPacket = queuePacket.front();
            if (av_packet_ref(packet, avPacket) == 0) {
                queuePacket.pop();
                ret = 0;
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                pthread_cond_signal(&condPacket);
            }
            break;
        } else {
            pthread_cond_wait(&condPacket, &mutexPacket);
        }
    }
    pthread_mutex_unlock(&mutexPacket);
    return ret;
}

int packet_queue::getQueueSize() {
    int size = 0;
    pthread_mutex_lock(&mutexPacket);
    size = queuePacket.size();
    pthread_mutex_unlock(&mutexPacket);
    return size;
}

void packet_queue::clearAll() {
    pthread_cond_signal(&condPacket);
    pthread_mutex_lock(&mutexPacket);

    while (!queuePacket.empty()) {
        AVPacket *packet = queuePacket.front();
        queuePacket.pop();
        av_packet_free(&packet);
        av_free(packet);
        packet = NULL;
    }
    pthread_mutex_unlock(&mutexPacket);
}

