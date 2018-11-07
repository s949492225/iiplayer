//
// Created by 宋林涛 on 2018/11/6.
//

#pragma once

#include <thread>
#include "../global/Status.h"
#include <cstdlib>
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

class AudioDecoder:public BaseDecoder {
private:
    void init();
    void decode();

public:
    AudioDecoder(MediaPlayer *player);
};

class HardVideoDecoder :public BaseDecoder{
private:
    void init();
    void decode();
    double getPacketDiffTime(AVPacket *packet);
public:
    HardVideoDecoder(MediaPlayer *player);

};
class VideoDecoder : public BaseDecoder {
private:
    void init();

    void decode();

public:
    VideoDecoder(MediaPlayer *player);
};

