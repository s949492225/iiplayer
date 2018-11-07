//
// Created by 宋林涛 on 2018/11/5.
//

#include "Decoder.h"
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
    int ret = 0;

    while (mPlayer->getStatus() != NULL && !mPlayer->getStatus()->isExit) {

        if (mPlayer->getStatus()->isSeek) {
            avcodec_flush_buffers(mPlayer->getHolder()->mAudioCodecCtx);
            av_usleep(1000 * 10);
            continue;
        }

        if (mPlayer->getStatus()->isPause) {
            av_usleep(1000 * 10);
            continue;
        }

        packet = av_packet_alloc();
        if (mQueue && mQueue->getPacket(packet) != 0) {
            av_packet_free(&packet);
            continue;
        }

        ret = avcodec_send_packet(mPlayer->getHolder()->mAudioCodecCtx, packet);
        if (ret != 0) {
            av_packet_free(&packet);
            continue;
        }

        while (ret >= 0) {
            AVFrame *frame = av_frame_alloc();
            ret = avcodec_receive_frame(mPlayer->getHolder()->mAudioCodecCtx, frame);
            if (ret >= 0) {

                if (frame->channels && frame->channel_layout == 0) {
                    frame->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(
                            frame->channels));
                } else if (frame->channels == 0 && frame->channel_layout > 0) {
                    frame->channels = av_get_channel_layout_nb_channels(
                            frame->channel_layout);
                }
                while (mPlayer->getStatus() != NULL && !mPlayer->getStatus()->isExit &&
                       mPlayer->getAudioRender()->isQueueFull()) {
                    av_usleep(1000 * 5);
                }
                if (mPlayer->getStatus() == NULL || mPlayer->getStatus()->isExit) {
                    av_frame_free(&frame);
                    break;
                }
                if (mPlayer->getAudioRender() && !mPlayer->getStatus()->isSeek) {
                    mPlayer->getAudioRender()->putFrame(frame);
                } else {
                    av_frame_free(&frame);
                }
            } else {
                av_frame_free(&frame);
            }
        }
        av_packet_free(&packet);
    }
}

