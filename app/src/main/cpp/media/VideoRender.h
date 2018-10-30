//
// Created by 宋林涛 on 2018/10/30.
//

#ifndef IIPLAYER_VIDEORENDER_H
#define IIPLAYER_VIDEORENDER_H

extern "C" {
#include <libavutil/frame.h>
}

class VideoRender {
public:
    void putFrame(AVFrame *frame);

    bool isQueueFull();
};


#endif //IIPLAYER_VIDEORENDER_H
