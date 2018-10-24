//
// Created by 宋林涛 on 2018/10/23.
//

#include "status.h"

status::status() {
    audio_packet_queue = new packet_queue(this);
    video_packet_queue = new packet_queue(this);
}

status::~status() {
    delete audio_packet_queue;
    delete video_packet_queue;
}
