//
// Created by 宋林涛 on 2018/11/6.
//

#include "Decoder.h"
#include "../MediaPlayer.h"

BaseDecoder::BaseDecoder(MediaPlayer *player) {
    mPlayer = player;
    mQueue = new PacketQueue(mPlayer->getStatus(), mPlayer->getHolder(),
                             const_cast<char *>("video"));
}

void BaseDecoder::start() {
    mDecodeThread = new std::thread([this] {
        this->decode();
    });
}

void BaseDecoder::putPacket(Packet *packet) {
    mQueue->putPacket(packet);
}

void BaseDecoder::clearQueue() {
    mQueue->clearAll();
}

void BaseDecoder::notifyWait() {
    mQueue->notifyAll();
}

int BaseDecoder::getQueueSize() {
    return mQueue->getQueueSize();
}

BaseDecoder::~BaseDecoder() {
    if (mDecodeThread != NULL) {
        try {
            mDecodeThread->join();
        }
        catch (std::exception &exception) {
            //ignore
        }
        if (mQueue!=NULL) {
            delete mQueue;
            mQueue=NULL;
        }
        mDecodeThread = NULL;
    }
    mPlayer = NULL;
}

