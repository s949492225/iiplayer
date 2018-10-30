//
// Created by 宋林涛 on 2018/10/23.
//

#ifndef IIPLAYER_STATUS_H
#define IIPLAYER_STATUS_H

#include "packet_queue.h"

class Status {
public:
    Status();

    ~Status();

    bool mExit = false;
    bool mPause = true;
    packet_queue *mAudioQueue = NULL;
    packet_queue *mVideoQueue = NULL;
    int mMaxQueueSize = 40;
    bool isSeek = false;
    bool isLoad = false;
};

#endif //IIPLAYER_STATUS_H
