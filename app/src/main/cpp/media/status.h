//
// Created by 宋林涛 on 2018/10/23.
//

#ifndef IIPLAYER_STATUS_H
#define IIPLAYER_STATUS_H

#include "packet_queue.h"

class status {
public:
    status();

    ~status();

    bool exit = false;
    bool pause = true;
    packet_queue *audio_packet_queue = NULL;
    packet_queue *video_packet_queue = NULL;
    int max_packet_queue_size = 40;
    bool seek = false;
    bool load = false;
};

#endif //IIPLAYER_STATUS_H
