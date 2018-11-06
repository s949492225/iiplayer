//
// Created by 宋林涛 on 2018/11/6.
//

#ifndef IIPLAYER_BASEDECODER_H
#define IIPLAYER_BASEDECODER_H

#include <thread>
#include "../global/Status.h"

extern "C" {
#include "libavutil/time.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}

class MediaPlayer;

class BaseDecoder {
protected:
    MediaPlayer *mPlayer = NULL;
    std::thread *mDecodeThread = NULL;
    PacketQueue *mQueue = NULL;

    virtual void start();
    virtual void decode() = 0;
public:
    BaseDecoder(MediaPlayer *player);
    virtual ~BaseDecoder();
    void putPacket(AVPacket *packet);
    void clearQueue();
    void notifyWait();
    int getQueueSize();
    virtual void init() = 0;
};


#endif //IIPLAYER_BASEDECODER_H
