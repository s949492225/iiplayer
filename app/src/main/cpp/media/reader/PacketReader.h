//
// Created by 宋林涛 on 2018/11/5.
//

#ifndef IIPLAYER_PACKETREADER_H
#define IIPLAYER_PACKETREADER_H

#include "../../android/android_log.h"
#include "../global/Status.h"
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
    std::thread *mReadThread = NULL;;
    pthread_mutex_t mReadMutex;
    MediaPlayer *mPlayer = NULL;

    void read();

    bool isHardCodec() const;

    void initBitStreamFilter();

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


};


#endif //IIPLAYER_PACKETREADER_H
