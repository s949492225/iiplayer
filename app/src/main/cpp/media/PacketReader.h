//
// Created by 宋林涛 on 2018/11/5.
//

#ifndef IIPLAYER_PACKETREADER_H
#define IIPLAYER_PACKETREADER_H

#include "../android/android_log.h"
#include "Status.h"
#include <thread>

extern "C" {
#include "libavutil/time.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}

class MediaPlayer;

class PacketReader {
private:
    AVFormatContext *mFormatCtx = NULL;
    std::thread *mReadThread = NULL;;
    pthread_mutex_t mMutexRead;
    pthread_cond_t mCondContinue;
    MediaPlayer *mPlayer = NULL;
    Status *mStatus = NULL;

    //audio
    int mAudioStreamIndex = -1;
    AVCodecContext *mAudioCodecCtx = NULL;
    //video
    int mVideoStreamIndex = -1;
    AVCodecContext *mVideoCodecCtx = NULL;
    void read();

public:
    PacketReader(MediaPlayer *player);

    ~PacketReader();

    int prepare();

    void handlerSeek();

    void checkBuffer(AVPacket *packet);

    void seekErrorPos(int sec);

    void setMediaInfo();

    void seek(int sec);

    void notifyWait();

    PacketQueue *mAudioQueue = NULL;
    PacketQueue *mVideoQueue = NULL;
};


#endif //IIPLAYER_PACKETREADER_H
