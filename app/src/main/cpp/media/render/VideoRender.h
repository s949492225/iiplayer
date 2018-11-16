//
// Created by 宋林涛 on 2018/10/30.
//

#ifndef IIPLAYER_VIDEORENDER_H
#define IIPLAYER_VIDEORENDER_H

#include <thread>
#include <unistd.h>
#include "../other/util.h"
#include "../queue/FrameQueue.h"
#include "../sdl/SDLVideo.h"


extern "C" {
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h>
}
class MediaPlayer;

class VideoRender {

private:
    MediaPlayer *mPlayer = NULL;
    AVPixelFormat mPixFmt;
    int mWidth = 0;
    int mHeight = 0;
    FrameQueue *mQueue = NULL;
    int mMaxQueueSize = 4;
    AVRational mTimebase;
    std::thread *mPlayThread = NULL;
    SDLVideo *mSDLVideo = NULL;
    pthread_mutex_t mRenderMutex;
    pthread_cond_t mRenderCond;
    bool isHand= false;

    pthread_mutex_t mGetSurfaseMutex;
    pthread_cond_t mGetSurfaceCond;
    int inited = 0;

    double getFrameDiffTime(AVFrame *avFrame);

    void renderOpenGL();

    void renderMediaCodec();

public:

    VideoRender(MediaPlayer *player, bool isHard);

    ~VideoRender();

    void putFrame(AVFrame *frame);

    bool isQueueFull();

    void play();

    void playThread();

    jobject getMediaCodecSurface(JNIEnv *pEnv);

    void clearQueue();

    void notifyWait();

    AVFrame *scale(AVFrame *frame);

    void onTextureReady();
};


#endif //IIPLAYER_VIDEORENDER_H
