//
// Created by 宋林涛 on 2018/9/28.
//

#include "frame_queue.h"
#include "status.h"

frame_queue::frame_queue(status *playStatus) {
    this->playStatus = playStatus;
    pthread_mutex_init(&mutexPacket, NULL);
    pthread_cond_init(&condPacket, NULL);
}

frame_queue::~frame_queue() {
    clearAll();
    pthread_mutex_destroy(&mutexPacket);
    pthread_cond_destroy(&condPacket);
}

int frame_queue::putFrame(AVFrame *packet) {
    pthread_mutex_lock(&mutexPacket);
    queuePacket.push(packet);
    pthread_cond_signal(&condPacket);
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int frame_queue::getFrame(AVFrame *frame) {
    pthread_mutex_lock(&mutexPacket);
    int ret = -1;
    while (playStatus != NULL && !playStatus->exit) {
        if (queuePacket.size() > 0) {
            AVFrame *avFrame = queuePacket.front();
            if (av_frame_ref(frame, avFrame) == 0) {
                queuePacket.pop();
                ret = 0;
                av_frame_free(&avFrame);
                av_free(avFrame);
                avFrame = NULL;
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

int frame_queue::getQueueSize() {
    int size = 0;
    pthread_mutex_lock(&mutexPacket);
    size = queuePacket.size();
    pthread_mutex_unlock(&mutexPacket);
    return size;
}

void frame_queue::clearAll() {
    pthread_cond_signal(&condPacket);
    pthread_mutex_lock(&mutexPacket);

    while (!queuePacket.empty()) {
        AVFrame *packet = queuePacket.front();
        queuePacket.pop();
        av_frame_free(&packet);
        av_free(packet);
        packet = NULL;
    }
    pthread_mutex_unlock(&mutexPacket);
}

