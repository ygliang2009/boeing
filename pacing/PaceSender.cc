#include "PaceSender.h"

PaceSender::PaceSender():mediaBudget(0, -1) {
    firstSentTs = -1;
    lastUpdateTs = BaseTimer::getCurrentTime();
    alr = new AlrDetector();
    mediaBudget.increaseBudget(MIN_PACKET_LIMIT_MS);  
}

PaceSender::~PaceSender() {
    if (alr != NULL) 
        delete alr;
}


bool PaceSender::setEstimateBitrate(const uint32_t &bitrateBps) {
    estimatedBitrate = bitrateBps;
    pacingBitrateKbps = BOE_MAX(bitrateBps / 1000, minSenderBitrateKbps) * DEFAULT_PACE_FACTOR;
    alr->setBitrate(bitrateBps);
    return true;
}

bool PaceSender::setBitrateLimits(const uint32_t &minSentBitrateBps) {
    minSenderBitrateKbps = minSentBitrateBps / 1000;
    pacingBitrateKbps = BOE_MAX(estimatedBitrate / 1000, minSenderBitrateKbps) * DEFAULT_PACE_FACTOR;
    return true;
}

/*将一个即将要发送的报文放入排队队列中*/
int PaceSender::insertPacket(\
    const uint32_t &seq, const int &retrans, const size_t &size, const int64_t &nowTs) {

    PacketEvent* ev = new PacketEvent();
    ev->seq = seq;
    ev->retrans = retrans;
    ev->size = size;
    ev->queueTs = nowTs;
    ev->sent = 0;
    return que.pushPacket(ev); 
}

int64_t PaceSender::getQueueMs() const {
    return BaseTimer::getCurrentTime() - que.getOldestTs();
}

size_t PaceSender::getQueueSize() const {
    return que.getQueueSize();
}

int64_t PaceSender::expectedQueueMs() const {
    if (pacingBitrateKbps > 0)
        return (que.getQueueSize() * 8) / pacingBitrateKbps;
    return 0; 
}

int64_t PaceSender::getLimitedStartTime() const {
    return alr->getAppLimitedStartedTs();
}

bool PaceSender::paceSend(PacketEvent *ev) {
    if (mediaBudget.budgetRemaining() == 0)
	return false;

    sendPacket(ev->seq, ev->retrans, ev->size);
    mediaBudget.useBudget(ev->size);
    return true;
}

bool PaceSender::sendPacket(const uint32_t &seq, const int &retrans, const size_t &size) {
    if (sender == NULL)
	return false;

    sender->sendCallback(seq, retrans, size);
    return true;
}

bool PaceSender::tryTransmit(const int64_t &nowTs) {
    int elapseMs, sentBytes = 0; 
    uint32_t targetBitrateKbps;
    //发送频率5ms
    elapseMs = nowTs - lastUpdateTs;
    if (elapseMs < MIN_PACKET_LIMIT_MS)
	return true;

    elapseMs = BOE_MIN(elapseMs, MAX_INTERVAL_MS);
    if (que.getQueueSize() > 0) {
        //estimate出来的码率和500ms内应发送的最大码率取一个最大值
        targetBitrateKbps = que.targetBitrateKbps(nowTs);
        targetBitrateKbps = BOE_MAX(pacingBitrateKbps, targetBitrateKbps) ;
    } else {
        targetBitrateKbps = pacingBitrateKbps;
    }
    if (!mediaBudget.setTargetRateKbps(targetBitrateKbps)) 
	return false;
    if (!mediaBudget.increaseBudget(elapseMs))
        return false;
    //发送速率控制
    PacketEvent* ev;
    while (!que.queueEmpty()) {
	//从cache中取数据发送
	ev = que.getQueueFront();

        if (paceSend(ev)) {
	    if (firstSentTs == -1)
	       firstSentTs = nowTs;

	    sentBytes += ev->size;
            //send完成后删除第一个元素，同时更新que信息
	    que.queueSent(ev);
        }
        else
	    //budget大小不够了
	    break;
    }
    lastUpdateTs = nowTs;
    
    if (sentBytes > 0)
        alr->bytesSent(sentBytes, elapseMs); 
    return true;
}

bool PaceSender::setSender(BoeSender *snd) {
    if (snd == NULL)
        return false;

    sender = snd;
    return true;
}

uint64_t PaceSender::getLimitedStartTime() {
    if (alr == NULL)
	return 0;

    return alr->getAppLimitedStartedTs();
}
