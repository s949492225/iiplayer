// Created by 宋林涛 on 2018/9/28.
//

#include "AudioRender.h"
#include "../MediaPlayer.h"
#include <cstdlib>

AudioRender::AudioRender(MediaPlayer *player) {
    mPlayer = player;
    mOutBuffer = (uint8_t *) (av_malloc((size_t) (SAMPLE_SIZE)));
    mAudio = new SDLAudio(mPlayer->getHolder()->mAudioCodecCtx->sample_rate);
    duration = mPlayer->getHolder()->mFormatCtx->duration;
    mTimebase = mPlayer->getHolder()->getAudioTimeBase();
    mQueue = new FrameQueue(mPlayer->getStatus(), const_cast<char *>("audio"));
    mOutChannelNum = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);

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
        //loading
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

        AVFrame *frame = mQueue->getFrame();
        if (frame != NULL) {
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
            break;
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

            if (render.mAudio != NULL) {
                render.mAudio->renderVoice(render.mOutBuffer,static_cast<SLuint32>(bufferSize));
            }

            if (render.mPlayer->getStatus()->isPlayEnd) {
                render.mPlayer->sendMsg(false, ACTION_PLAY_FINISH);
            }
        }
    }

}

void AudioRender::createPlayer() {
    if (mAudio != NULL) {
        mAudio->create(renderAudioCallBack, this);
    }
}


void AudioRender::pause() {
    if (mAudio != NULL) {
        mAudio->pause();
    }
}

void AudioRender::resume() {
    if (mAudio != NULL) {
        mAudio->resume();
    }
}

AudioRender::~AudioRender() {
    if (mPlayThread != NULL) {
        mPlayThread->join();
        mPlayThread = NULL;
    }

    if (mAudio != NULL) {
        delete mAudio;
        mAudio = NULL;
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
