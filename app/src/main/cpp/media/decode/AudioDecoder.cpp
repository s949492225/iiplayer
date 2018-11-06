//
// Created by 宋林涛 on 2018/11/5.
//

#include "AudioDecoder.h"
#include "../MediaPlayer.h"

AudioDecoder::AudioDecoder(MediaPlayer *player) : BaseDecoder(player) {

}

void AudioDecoder::init() {
    start();
}

void AudioDecoder::decode() {
    if (LOG_DEBUG) {
        LOGD("音频解码线程开始,tid:%i\n", gettid())
    }
    AVPacket *packet = NULL;
    AVFrame *frame = NULL;
    int ret = 0;

    while (mPlayer->mStatus != NULL && !mPlayer->mStatus->isExit) {

        if (mPlayer->mStatus->isSeek) {
            av_usleep(1000 * 100);
            continue;
        }

        if (mPlayer->mStatus->isPause) {
            av_usleep(1000 * 100);
            continue;
        }

        packet = av_packet_alloc();
        if (mQueue && mQueue->getPacket(packet) != 0) {
            av_packet_free(&packet);
            continue;
        }

        ret = avcodec_send_packet(mPlayer->mHolder->mAudioCodecCtx, packet);
        if (ret != 0) {
            av_packet_free(&packet);
            continue;
        }

        frame = av_frame_alloc();
        ret = avcodec_receive_frame(mPlayer->mHolder->mAudioCodecCtx, frame);
        if (ret == 0) {

            if (frame->channels && frame->channel_layout == 0) {
                frame->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(
                        frame->channels));
            } else if (frame->channels == 0 && frame->channel_layout > 0) {
                frame->channels = av_get_channel_layout_nb_channels(
                        frame->channel_layout);
            }
            while (mPlayer->mStatus != NULL && !mPlayer->mStatus->isExit &&
                   mPlayer->getAudioRender()->isQueueFull()) {
                av_usleep(1000 * 5);
            }
            if (mPlayer->mStatus == NULL || mPlayer->mStatus->isExit) {
                av_frame_free(&frame);
                continue;
            }
            if (mPlayer->getAudioRender() && !mPlayer->mStatus->isSeek) {
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

