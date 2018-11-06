//
// Created by 宋林涛 on 2018/11/5.
//

#ifndef IIPLAYER_VIDEODECODER_H
#define IIPLAYER_VIDEODECODER_H

#include "BaseDecoder.h"


class VideoDecoder : public BaseDecoder {
private:
    void init();

    void decode();

public:
    VideoDecoder(MediaPlayer *player);
};


#endif //IIPLAYER_VIDEODECODER_H
