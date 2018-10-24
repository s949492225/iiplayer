//
// Created by 宋林涛 on 2018/10/22.
//

#include "media_player.h"

void media_player::send_msg(int type) {
    LOGD("消息内容:%i\n", type);
}

int read_interrupt_callback(void *ctx) {
    media_player *player = static_cast<media_player *>(ctx);
    if (player->play_status->exit) {
        return AVERROR_EOF;
    }
    return 0;
}

media_player::media_player() {
}

void media_player::open(const char *url) {
    LOGD("media_player open invoked,url:%s\n", url)
    this->url = url;
    this->play_status = new status();
    t_read = new std::thread(std::bind(&media_player::read_thread, this));
}

void media_player::start() {
    play_status->pause = false;
}

void media_player::read_thread() {
    LOGD("读取线程启动成功,tid:%i\n", gettid());
    int error_code = prepare();
    if (error_code < 0) {
        return;
    }
    //read packet
    while (play_status != NULL && !play_status->exit) {
        if (play_status->pause) {
            av_usleep(1000 * 100);
            continue;
        }
        AVPacket *packet = av_packet_alloc();
        if (av_read_frame(pFormatCtx, packet) >= 0) {
            if (packet->stream_index == audio_stream_index) {
                while (play_status->audio_packet_queue->getQueueSize() >=
                       play_status->max_packet_queue_size) {
                    av_usleep(1000 * 5);
                }
                play_status->audio_packet_queue->putPacket(packet);
            } else {
                av_packet_free(&packet);
                av_free(packet);
            }
        } else {
            //播放完成
            av_packet_free(&packet);
            av_free(packet);
            break;
        }
    }


}


void media_player::decode_audio() {
    if (LOG_DEBUG) {
        LOGD("音频解码线程开始,tid:%i\n", gettid())
    }

    AVPacket *packet = NULL;
    AVFrame *audio_frame = NULL;
    int ret = 0;

    while (play_status != NULL && !play_status->exit) {
        if (play_status->pause) {
            av_usleep(1000 * 100);
            continue;
        }
        packet = av_packet_alloc();
        if (play_status->audio_packet_queue->getPacket(packet) != 0) {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            continue;
        }
        ret = avcodec_send_packet(audio_codec_ctx, packet);
        if (ret != 0) {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            continue;
        }

        audio_frame = av_frame_alloc();
        ret = avcodec_receive_frame(audio_codec_ctx, audio_frame);
        if (ret == 0) {

            if (audio_frame->channels && audio_frame->channel_layout == 0) {
                audio_frame->channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(
                        audio_frame->channels));
            } else if (audio_frame->channels == 0 && audio_frame->channel_layout > 0) {
                audio_frame->channels = av_get_channel_layout_nb_channels(
                        audio_frame->channel_layout);
            }
            LOGD("audio decode time :%lld\n", audio_frame->pts)
            while (a_render->audio_frame_queue->getQueueSize() >= a_render->max_frame_queue_size) {
                av_usleep(1000 * 5);
            }
            a_render->audio_frame_queue->putFrame(audio_frame);
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
        } else {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            LOGD("播放完成\n");
        }
    }
}


int media_player::prepare() {

    av_register_all();
    avformat_network_init();

    pFormatCtx = avformat_alloc_context();
    pFormatCtx->interrupt_callback.callback = read_interrupt_callback;
    pFormatCtx->interrupt_callback.opaque = this;

    int error = avformat_open_input(&pFormatCtx, url, NULL, NULL);
    if (error != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open url %s", url);
        }
        send_msg(ERROR_OPEN_FILE);
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not find steams from %s", url);
        }
        send_msg(ERROR_FIND_STREAM);
        return -1;
    }
    //流信息获取
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        AVCodecParameters *parameters = pFormatCtx->streams[i]->codecpar;
        if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            //音频解码器
            if (i != -1) {
                a_render = new audio_render(play_status, parameters->sample_rate,
                                            pFormatCtx->streams[i]->time_base);
                audio_stream_index = i;
                duration = static_cast<int>(pFormatCtx->duration / AV_TIME_BASE);
                get_codec_context(parameters, &audio_codec_ctx);
            }
        } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (i != -1) {
                video_stream_index = i;
                video_timebase = pFormatCtx->streams[i]->time_base;
                get_codec_context(parameters, &video_codec_ctx);
            }
        }
    }

    send_msg(SUCCESS_PREPARED);
    //start audio decode thread
    t_audio_decode = new std::thread(std::bind(&media_player::decode_audio, this));
    return 1;
}

void media_player::stop() {
    play_status->exit = true;

    t_read->join();
    t_audio_decode->join();

    delete a_render;
    delete t_read;
    delete t_audio_decode;
    delete play_status;


}

void media_player::play() {
    play_status->pause = false;
    a_render->play();
}
