//
// Created by 宋林涛 on 2018/11/19.
//

#ifndef IIPLAYER_SDLAUDIO_H
#define IIPLAYER_SDLAUDIO_H

#include <cstdlib>

extern "C" {
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
}

class SDLAudio {
private:
    // 引擎接口
    SLObjectItf mEngineObj = NULL;
    SLEngineItf mEngineIF = NULL;

    //混音器
    SLObjectItf mMixObj = NULL;
    SLEnvironmentalReverbItf mMixEnvReverbIF = NULL;
    SLEnvironmentalReverbSettings mReverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    //pcm player
    SLObjectItf mPlayerObj = NULL;
    SLPlayItf mPlayerIF = NULL;
    //voice
    SLVolumeItf mVolumeIF = NULL;

    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf mBufferQueue = NULL;

    int mySampleRate = 0;
public:
    SDLAudio(int sampleRate);

    ~SDLAudio();

    void create(slAndroidSimpleBufferQueueCallback callback, void *host);

    void pause();

    void resume();

    void renderVoice(void *buffer,SLuint32 bufferSize);
};


#endif //IIPLAYER_SDLAUDIO_H
