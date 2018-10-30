//
// Created by 宋林涛 on 2018/10/23.
//

#include "Status.h"

Status::Status() {
    mAudioQueue = new packet_queue(this);
    mVideoQueue = new packet_queue(this);
}

Status::~Status() {
    delete mAudioQueue;
    delete mVideoQueue;
}
