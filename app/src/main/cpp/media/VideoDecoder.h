//
// Created by 宋林涛 on 2018/11/5.
//

#ifndef IIPLAYER_VIDEODECODER_H
#define IIPLAYER_VIDEODECODER_H

#include <thread>
#include "Status.h"

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
    Status *mStatus = NULL;
    std::thread *mDecodeThread = NULL;
    AVCodecContext *mCoderCtx = NULL;
    PacketQueue *mQueue = NULL;

    void decode();

public:
    VideoDecoder(MediaPlayer *player, PacketQueue *pQueue);

    ~VideoDecoder();

    void start(AVCodecContext *pContext);
};


#endif //IIPLAYER_VIDEODECODER_H
