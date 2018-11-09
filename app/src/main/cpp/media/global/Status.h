//
// Created by 宋林涛 on 2018/10/23.
//

#ifndef IIPLAYER_STATUS_H
#define IIPLAYER_STATUS_H

#include "../queue/PacketQueue.h"

class Status {
public:
    bool isExit = false;
    bool isPause = true;
    bool isSeek = false;
    bool isLoad = false;
    bool isPlayEnd = false;
    bool isEOF = false;
    int mSeekSec = 0;
};

#endif //IIPLAYER_STATUS_H
