//
// Created by 宋林涛 on 2018/11/5.
//

#ifndef IIPLAYER_AUDIODECODER_H
#define IIPLAYER_AUDIODECODER_H


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

class AudioDecoder {
private:
    MediaPlayer *mPlayer = NULL;
    std::thread *mDecodeThread = NULL;

    void decode();

public:
    AudioDecoder(MediaPlayer *player);

    ~AudioDecoder();

    void start();

    PacketQueue *mQueue = NULL;
};


#endif //IIPLAYER_AUDIODECODER_H
