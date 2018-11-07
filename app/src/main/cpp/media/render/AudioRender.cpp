// Created by 宋林涛 on 2018/9/28.
//

#include "AudioRender.h"
#include "../MediaPlayer.h"
#include <cstdlib>

AudioRender::AudioRender(MediaPlayer *player) {
    mPlayer = player;
    this->duration = mPlayer->getHolder()->mFormatCtx->duration;
    mSampleRate = mPlayer->getHolder()->mAudioCodecCtx->sample_rate;
    mTimebase = mPlayer->getHolder()->getAudioTimeBase();
    mQueue = new FrameQueue(mPlayer->getStatus(), const_cast<char *>("audio"));
    mOutChannelNum = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    mOutBuffer = (uint8_t *) (av_malloc((size_t) (SAMPLE_SIZE)));
    initSwrCtx(mPlayer->getHolder()->mAudioCodecCtx);
}

void AudioRender::initSwrCtx(AVCodecContext *context) {
    mSwrCtx = swr_alloc();
    swr_alloc_set_opts(mSwrCtx,
                       AV_CH_LAYOUT_STEREO,
                       AV_SAMPLE_FMT_S16,
                       context->sample_rate,
                       context->channel_layout,
                       context->sample_fmt,
                       context->sample_rate,
                       NULL, NULL);
    swr_init(mSwrCtx);
}

void AudioRender::playThread() {
    LOGD("音频播放线程开始,tid:%i\n", gettid())
    createPlayer();
}

void AudioRender::play() {
    mPlayThread = new std::thread(std::bind(&AudioRender::playThread, this));
}

int AudioRender::getPcmData() {
    mOutSize = 0;
    while (mPlayer->getStatus() != NULL && !mPlayer->getStatus()->isExit) {
        //为seek清理异常数据
        if (mPlayer->getStatus()->isSeek) {
            pthread_mutex_lock(&mPlayer->getHolder()->mSeekMutex);
            mPlayer->getStatus()->mSeekReadyCount += 1;
            pthread_cond_wait(&mPlayer->getHolder()->mSeekCond, &mPlayer->getHolder()->mSeekMutex);
            pthread_mutex_unlock(&mPlayer->getHolder()->mSeekMutex);
            //clear
            clearQueue();
            continue;
        }

        //加载中
        if (mQueue && mQueue->getQueueSize() == 0) {

            if (!mPlayer->getStatus()->isLoad) {
                mPlayer->getStatus()->isLoad = true;
                mPlayer->sendMsg(false, ACTION_PLAY_LOADING);
            }
            av_usleep(1000 * 10);
            continue;
        } else {
            if (mPlayer->getStatus()->isLoad) {
                mPlayer->getStatus()->isLoad = false;
                mPlayer->sendMsg(false, ACTION_PLAY_LOADING_OVER);
            }
        }

        AVFrame *frame = av_frame_alloc();
        int ret = mQueue->getFrame(frame);
        if (ret == 0) {

            if (frame->channels && frame->channel_layout == 0) {
                frame->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(
                        frame->channels));
            } else if (frame->channels == 0 && frame->channel_layout > 0) {
                frame->channels = av_get_channel_layout_nb_channels(frame->channel_layout);
            }

            int nb = swr_convert(mSwrCtx, &mOutBuffer, SAMPLE_SIZE,
                                 (const uint8_t **) frame->data, frame->nb_samples);

            mOutSize = nb * mOutChannelNum * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
            mPlayer->setClock(frame->pts * av_q2d(mTimebase));
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
            break;
        } else {
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
        }
    }
    return mOutSize;
}

void renderAudioCallBack(SLAndroidSimpleBufferQueueItf  __unused queue, void *data) {
    if (data != NULL) {
        AudioRender &render = *((AudioRender *) data);
        int bufferSize = render.getPcmData();
        if (bufferSize > 0) {
            render.mPlayer->setClock(
                    render.mPlayer->getClock() + bufferSize / ((double) SAMPLE_SIZE));
            double diff = render.mPlayer->getClock() - render.mPlayer->getDuration();
            if (diff > -0.05 && diff < 10 && render.mQueue->getQueueSize() == 0) {
                render.mPlayer->getStatus()->isPlayEnd = true;
            }
            render.mPlayer->sendMsg(false, DATA_NOW_PLAYING_TIME, (int) render.mPlayer->getClock());
            SLresult result = (*render.mBufferQueue)->Enqueue(render.mBufferQueue,
                                                              (char *) render.mOutBuffer,
                                                              static_cast<SLuint32>(bufferSize));
            if (result != SL_RESULT_SUCCESS) {
                LOGE("音频渲染出错\n");
            }
            if (render.mPlayer->getStatus()->isPlayEnd) {
                render.mPlayer->sendMsg(false, ACTION_PLAY_FINISH);
            }
        }
    }

}

void AudioRender::createPlayer() {

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
    result = (*mBufferQueue)->RegisterCallback(mBufferQueue, renderAudioCallBack, this);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 RegisterCallback\n");
    }
    (void) result;
//    获取播放状态接口
    result = (*mPlayerIF)->SetPlayState(mPlayerIF, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("音频渲染出错 SetPlayState\n");
    }
    (void) result;
    renderAudioCallBack(mBufferQueue, this);

}


void AudioRender::pause() {
    if (mPlayerIF != NULL) {
        (*mPlayerIF)->SetPlayState(mPlayerIF, SL_PLAYSTATE_PAUSED);
    }
}

void AudioRender::resume() {
    if (mPlayerIF != NULL) {
        (*mPlayerIF)->SetPlayState(mPlayerIF, SL_PLAYSTATE_PLAYING);
    }
}

AudioRender::~AudioRender() {
    if (mPlayThread != NULL) {
        mPlayThread->join();
        mPlayThread = NULL;
    }

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

    if (mOutBuffer != NULL) {
        delete (mOutBuffer);
        mOutBuffer = NULL;
    }

    if (mSwrCtx != NULL) {
        swr_free(&mSwrCtx);
        mSwrCtx = NULL;
    }

    if (mQueue != NULL) {
        delete mQueue;
        mQueue = NULL;
    }
}

bool AudioRender::isQueueFull() {
    return mQueue->getQueueSize() >= mMaxQueueSize;
}

void AudioRender::notifyWait() {
    mQueue->notifyAll();
}

void AudioRender::putFrame(AVFrame *frame) {
    if (mQueue) {
        mQueue->putFrame(frame);
    }
}

void AudioRender::clearQueue() {
    mQueue->clearAll();
}
