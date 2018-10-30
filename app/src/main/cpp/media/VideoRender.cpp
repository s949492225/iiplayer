//
// Created by 宋林涛 on 2018/10/30.
//

#include "VideoRender.h"

void VideoRender::putFrame(AVFrame *frame) {
    av_frame_free(&frame);
    av_free(frame);
    frame = NULL;
}

bool VideoRender::isQueueFull() {
    return false;
}
