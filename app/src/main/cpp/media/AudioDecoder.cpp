//
// Created by 宋林涛 on 2018/11/5.
//

#include "AudioDecoder.h"
#include "MediaPlayer.h"

AudioDecoder::AudioDecoder(MediaPlayer *player) {
    mPlayer = player;
    mStatus = player->mStatus;
}

AudioDecoder::~AudioDecoder() {
    mDecodeThread->join();
    mDecodeThread = NULL;
    mPlayer = NULL;
    mStatus = NULL;
    mCoderCtx = NULL;
}

void AudioDecoder::start(AVCodecContext *pContext) {
    mCoderCtx = pContext;
    mDecodeThread = new std::thread(std::bind(&AudioDecoder::decodeAudio, this));
}

void AudioDecoder::decodeAudio() {
    if (LOG_DEBUG) {
        LOGD("音频解码线程开始,tid:%i\n", gettid())
    }
    AVPacket *packet = NULL;
    AVFrame *frame = NULL;
    int ret = 0;

    while (mStatus != NULL && !mStatus->isExit) {

        if (mStatus->isSeek) {
            av_usleep(1000 * 100);
            continue;
        }

        if (mStatus->isPause) {
            av_usleep(1000 * 100);
            continue;
        }

        packet = av_packet_alloc();
        if (mStatus->mAudioQueue->getPacket(packet) != 0) {
            av_packet_free(&packet);
            continue;
        }
        ret = avcodec_send_packet(mCoderCtx, packet);
        if (ret != 0) {
            av_packet_free(&packet);
            continue;
        }

        frame = av_frame_alloc();
        ret = avcodec_receive_frame(mCoderCtx, frame);
        if (ret == 0) {

            if (frame->channels && frame->channel_layout == 0) {
                frame->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(
                        frame->channels));
            } else if (frame->channels == 0 && frame->channel_layout > 0) {
                frame->channels = av_get_channel_layout_nb_channels(
                        frame->channel_layout);
            }
            while (mStatus != NULL && !mStatus->isExit &&
                   mPlayer->getAudioRender()->isQueueFull()) {
                av_usleep(1000 * 5);
            }
            if (mStatus == NULL || mStatus->isExit) {
                av_frame_free(&frame);
                continue;
            }
            if (!mStatus->isSeek) {
                mPlayer->getAudioRender()->putFrame(frame);
            } else {
                av_frame_free(&frame);
            }
            av_packet_free(&packet);
        } else {
            av_frame_free(&frame);
            av_packet_free(&packet);
        }
    }
}
