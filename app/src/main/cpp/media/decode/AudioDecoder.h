//
// Created by 宋林涛 on 2018/11/5.
//

#ifndef IIPLAYER_AUDIODECODER_H
#define IIPLAYER_AUDIODECODER_H


#include "BaseDecoder.h"

class AudioDecoder:public BaseDecoder {
private:
    void init();
    void decode();

public:
    AudioDecoder(MediaPlayer *player);
};


#endif //IIPLAYER_AUDIODECODER_H
