//
// Created by 宋林涛 on 2018/10/23.
//

#include "Status.h"

Status::Status() {
    pthread_cond_init(&mCondRead, NULL);
    mAudioQueue = new PacketQueue(this, "audio");
    mVideoQueue = new PacketQueue(this, "video");
}

Status::~Status() {
    delete mAudioQueue;
    delete mVideoQueue;
    pthread_cond_destroy(&mCondRead);
}
