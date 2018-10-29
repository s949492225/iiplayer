//
// Created by 宋林涛 on 2018/10/22.
//

#ifndef IIPLAYER_MEDIA_PALYER_H
#define IIPLAYER_MEDIA_PALYER_H

#include "../android/android_log.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include "util.h"
#include "status.h"
#include "audio_render.h"
#include <unistd.h>
#include "time.h"

extern "C" {
#include "libavutil/time.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}

class media_player {
public:
    media_player();

    status *play_status = NULL;
    std::thread *t_read = NULL;
    std::thread *t_audio_decode = NULL;

    AVFormatContext *pFormatCtx = NULL;
    const char *url = NULL;
    int duration = 0;

    //audio
    int audio_stream_index = -1;
    AVCodecContext *audio_codec_ctx = NULL;
    audio_render *a_render = NULL;

    //video
    int video_stream_index = -1;
    AVCodecContext *video_codec_ctx = NULL;
    void *msg_sender = NULL;

    int prepare();

    void read_thread();

    void open(const char *string);

    void play();

    void pause();

    void resume();

    void stop();

private:
    void decode_audio();

    void send_msg(int type);

    void send_jni_msg(int type) const;
};


#endif //IIPLAYER_MEDIA_PALYER_H
