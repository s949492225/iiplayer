//
// Created by 宋林涛 on 2018/11/6.
//

#ifndef IIPLAYER_HARDVIDEODECODER_H
#define IIPLAYER_HARDVIDEODECODER_H

#include "../queue/PacketQueue.h"
#include "BaseDecoder.h"

class MediaPlayer;
class HardVideoDecoder :public BaseDecoder{
private:
    void init();
    void decode();
    double getPacketDiffTime(AVPacket *packet);
public:
    HardVideoDecoder(MediaPlayer *player);

};


#endif //IIPLAYER_HARDVIDEODECODER_H
