//
// Created by 宋林涛 on 2018/11/19.
//

#include "SDLAudio.h"
#include "../../android/android_log.h"

SDLAudio::SDLAudio(int sampleRate) {
    mSampleRate = sampleRate;
}

void SDLAudio::create(slAndroidSimpleBufferQueueCallback callback, void *host) {
    SLresult result;
    result = slCreateEngine(&mEngineObj, 0, 0, 0, 0, 0);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*engineObject)->Realize(engineObject\n");
    }
    (void) result;
    result = (*mEngineObj)->Realize(mEngineObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*engineObject)->Realize(engineObject\n");
    }
    (void) result;
    result = (*mEngineObj)->GetInterface(mEngineObj, SL_IID_ENGINE, &mEngineIF);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*engineObject)->GetInterface(engineObject\n");
    }
    (void) result;
    //第二步，创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    result = (*mEngineIF)->CreateOutputMix(mEngineIF, &mMixObj, 1, mids, mreq);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*engineEngine)->CreateOutputMix\n");
    }
    (void) result;
    result = (*mMixObj)->Realize(mMixObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*outputMixObject)->Realize(outputMixObject\n");
    }
    (void) result;
    result = (*mMixObj)->GetInterface(mMixObj, SL_IID_ENVIRONMENTALREVERB,
                                      &mMixEnvReverbIF);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*outputMixObject)->GetInterface\n");
    }
    (void) result;
    if (SL_RESULT_SUCCESS == result) {
        result = (*mMixEnvReverbIF)->SetEnvironmentalReverbProperties(
                mMixEnvReverbIF, &mReverbSettings);
        if (result != SL_RESULT_SUCCESS) {
            LOGE("音频渲染出错 SetEnvironmentalReverbProperties\n");
        }
        (void) result;
    }
    (void) result;
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, mMixObj};
    SLDataSink audioSnk = {&outputMix, 0};


    // 第三步，配置PCM格式信息
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};

    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//播放pcm格式的数据
            2,//2个声道（立体声）
            static_cast<SLuint32>(mSampleRate * 1000),//44100hz的频率
            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN//结束标志
    };
    SLDataSource slDataSource = {&android_queue, &pcm};


    const SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_PLAYBACKRATE};
    const SLboolean req[2] = {SL_RESULT_SUCCESS, SL_RESULT_SUCCESS};

    result = (*mEngineIF)->CreateAudioPlayer(mEngineIF, &mPlayerObj, &slDataSource,
                                             &audioSnk, 2,
                                             ids, req);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 CreateAudioPlayer\n");
    }
    (void) result;
    //初始化播放器
    result = (*mPlayerObj)->Realize(mPlayerObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*pcmPlayerObject)->Realize(pcmPlayerObject\n");
    }
    (void) result;
//    得到接口后调用  获取Player接口
    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_PLAY, &mPlayerIF);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*pcmPlayerObject)->SL_IID_PLAY\n");
    }
    (void) result;
//    注册回调缓冲区 获取缓冲队列接口
    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_BUFFERQUEUE, &mBufferQueue);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 (*pcmPlayerObject)->SL_IID_BUFFERQUEUE\n");
    }
    (void) result;
    //缓冲接口回调
    result = (*mBufferQueue)->RegisterCallback(mBufferQueue, callback, host);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 RegisterCallback\n");
    }
    (void) result;
    //获取播放状态接口
    result = (*mPlayerIF)->SetPlayState(mPlayerIF, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 SetPlayState\n");
    }
    (void) result;
    callback(mBufferQueue, host);

}

void SDLAudio::pause() {
    if (mPlayerIF != NULL) {
        (*mPlayerIF)->SetPlayState(mPlayerIF, SL_PLAYSTATE_PAUSED);
    }
}

void SDLAudio::resume() {
    if (mPlayerIF != NULL) {
        (*mPlayerIF)->SetPlayState(mPlayerIF, SL_PLAYSTATE_PLAYING);
    }
}

SDLAudio::~SDLAudio() {
    if (mPlayerObj != NULL) {
        (*mPlayerObj)->Destroy(mPlayerObj);
        mPlayerObj = NULL;
        mPlayerIF = NULL;
        mBufferQueue = NULL;
    }

    if (mMixObj != NULL) {
        (*mMixObj)->Destroy(mMixObj);
        mMixObj = NULL;
        mMixEnvReverbIF = NULL;
    }

    if (mEngineObj != NULL) {
        (*mEngineObj)->Destroy(mEngineObj);
        mEngineObj = NULL;
        mEngineIF = NULL;
    }
}

void SDLAudio::renderVoice(void *buffer,SLuint32 bufferSize) {
    SLresult result = (*mBufferQueue)->Enqueue(mBufferQueue,
                                               buffer,
                                               bufferSize);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错\n");
    }
}

