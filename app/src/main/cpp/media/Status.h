//
// Created by 宋林涛 on 2018/10/23.
//

#ifndef IIPLAYER_STATUS_H
#define IIPLAYER_STATUS_H

#include "PacketQueue.h"

class Status {
public:
    Status();

    ~Status();

    bool isExit = false;
    bool isPause = true;
    PacketQueue *mAudioQueue = NULL;
    PacketQueue *mVideoQueue = NULL;
    int mMaxQueueSize = 40;
    bool isSeek = false;
    bool isLoad = false;
    int mSeekSec = 0;
};

#endif //IIPLAYER_STATUS_H
