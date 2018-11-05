//
// Created by 宋林涛 on 2018/11/5.
//

#ifndef IIPLAYER_VIDEODECODER_H
#define IIPLAYER_VIDEODECODER_H

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

class VideoDecoder {
private:
    MediaPlayer *mPlayer = NULL;
    std::thread *mDecodeThread = NULL;
    void decode();

public:
    VideoDecoder(MediaPlayer *player);

    ~VideoDecoder();

    void start();

    PacketQueue *mQueue = NULL;
};


#endif //IIPLAYER_VIDEODECODER_H
