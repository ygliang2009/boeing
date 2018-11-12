#include "Sender.h"

BoeSender::BoeSender() {
    limiter = new SenderLimiter();

    scc = new BoeSendCongestionCtrl();
    scc->registerTrigger(this);

    firstTs = -1;
    actived = 0;
    basePacketId = 0;
    packetIdSeed = 0;
    frameIdSeed = 0;
    transportSeqSeed = 0;
}

BoeSender::~BoeSender() {
    CLEANUP(limiter);
    CLEANUP(scc);
}

bool BoeSender::procSenderHeartbeat(int32_t currTs) {
    if (scc == NULL)
        return false;

    scc->procSccHeartbeat();
    return true;
}

bool BoeSender::registerCallerCallback(Caller *call) {
    if (this->caller != NULL)
        this->caller = call;

    return true;
}

/*每次码率变更后会通知到这个函数，主要是FEC和重传的逻辑，后续补充*/
bool BoeSender::networkChangeTrigger(uint32_t bitrate, uint8_t fractionLoss, uint32_t rtt) {
    
    if (this->caller == NULL) 
        return false;

    TargetTransferRate transferRateMsg;
    transferRateMsg.atTime = BaseTimer::getCurrentTime();
    transferRateMsg.targetRate = bitrate;
    transferRateMsg.lossRateRatio = fractionLoss;
    transferRateMsg.roundTripTime = rtt;
 
    caller->onTargetTransferRate(transferRateMsg);
    return true;
}

/*更新scc的rtt值，rtt值传入到onLossInfoResult函数中*/
bool BoeSender::updateRtt(const uint32_t rtt) const {
    if (scc == NULL)
        return false;
    
    if (!scc->updateRtt(rtt)) {
	return false;
    }
    return true;
}

/*将视频帧拆解成packet进行发送*/
bool BoeSender::addPackets(\
        const uint8_t payloadType, const uint8_t ftype, const uint8_t* data, const size_t size) {

    BoeSegmentMessage *segMsg;
    int64_t nowTs;
    uint16_t splits[SPLIT_NUMBER], total, i;
    uint32_t key; 
    uint32_t timestamp; 

    nowTs = BaseTimer::getCurrentTime();
    total = __splitFrame(splits, size);
    /*计算时间戳*/
    if (firstTs == -1) {
	timestamp = 0;
	firstTs = nowTs;
    } else {
	timestamp = (uint32_t)(nowTs - firstTs);
    }
    const uint8_t *pos = data;
    ++frameIdSeed;

    for (i = 0; i < total; i++) {
	segMsg = new BoeSegmentMessage();

	segMsg->packetId = ++packetIdSeed;
	segMsg->fid = frameIdSeed;
	segMsg->interval = timestamp;
        segMsg->ftype = ftype;
    	segMsg->payloadType = payloadType;
	segMsg->index = i;
        segMsg->total = total;
        /*发送时间暂时设定为0，等到报文真正发送时才填充其值*/
        segMsg->sendInterval = 0;
        /*发送侧唯一ID，在发送时填充*/
        segMsg->transportSeq = 0;
	segMsg->dataSize = splits[i];
	memcpy(segMsg->data, pos, splits[i]);
	pos += splits[i];
	
        key = segMsg->packetId;
	sendingCache[key] = segMsg;
	scc->addPacket(key, 0, segMsg->dataSize + SEGMENT_HEADER_SIZE);
    }
    return true;
}

uint16_t BoeSender::__splitFrame(uint16_t splits[], size_t size) {
    uint16_t ret, i;
    uint16_t remainSize;

    if (size <= MAX_PACKET_SIZE) {
	ret = 1;
	splits[0] = size;
    } else {
	ret = size / MAX_PACKET_SIZE;
	for (i = 0; i < ret; i++) {
	    splits[i] = MAX_PACKET_SIZE;
	}
        remainSize = size % MAX_PACKET_SIZE; 
	if (remainSize > 0) {
	    splits[ret] = remainSize;
	    ret++;
        }
    }
    return ret;
}

bool BoeSender::procSegmentAck(BoeSegmentAckMessage *segmentAckMsg) {
    uint64_t i;

    if (segmentAckMsg == NULL)
	return false;
    /*检查当前确认的id是否大于当前发送的最大id*/
    if (segmentAckMsg->ackedPacketId > packetIdSeed || segmentAckMsg->basePacketId > packetIdSeed) {
	return false;
    }
    /*根据接收端反馈的basePacket信息，更新发送端的发送缓存，将basePacketId之前的序号都删除掉 */
    __upCacheAccodingBase(segmentAckMsg->basePacketId); 
   
    if (limiter == NULL)
        return false;

    uint64_t nowTs = BaseTimer::getCurrentTime();
    limiter->limiterUpdate(0, nowTs);
    /*处理NACK消息*/
    for (i = 0; i < segmentAckMsg->nackNum; ++i) {
        uint32_t nackPacketId = segmentAckMsg->basePacketId + segmentAckMsg->nack[i];
        /*从cache中找出nack packet，重传*/
        std::map<uint64_t, BoeSegmentMessage *>::iterator sendingIter = sendingCache.find(nackPacketId);

        if (sendingIter != sendingCache.end()) {
            if (limiter->limiterTrySend(sendingIter->second->dataSize + SEGMENT_HEADER_SIZE, nowTs)) {
       	 /* 因为是重发，所以retrans参数设置为1 */
       	 scc->addPacket(sendingIter->second->packetId, 1, sendingIter->second->dataSize + SEGMENT_HEADER_SIZE);
       	 
       	 limiter->limiterUpdate(sendingIter->second->dataSize + SEGMENT_HEADER_SIZE, nowTs);
            } 
        }
    }

    /*更新RTT信息*/
    nowTs = BaseTimer::getCurrentTime();
    std::map<uint64_t, BoeSegmentMessage *>::iterator segIter = sendingCache.find(segmentAckMsg->ackedPacketId);
    if (segIter != sendingCache.end()) {
        BoeSegmentMessage *segMsg = segIter->second;
        if (nowTs > (segMsg->sendInterval - segMsg->interval - firstTs)) {
            session->sessCalculateRtt(nowTs - segMsg->sendInterval - segMsg->interval - firstTs); 
        }
        /*删除SegmentMessage占用的空间*/
        delete segIter->second;
       /*删除map内存块占用的空间*/
        sendingCache.erase(segmentAckMsg->ackedPacketId);
    } 
    return true;
}

/*根据发送来的basePacketId更新cache的值*/
bool BoeSender::__upCacheAccodingBase(const uint32_t ackBasedId) {
    std::map<uint64_t, BoeSegmentMessage* >::iterator sendingIter = sendingCache.find(basePacketId);
    
    while ((sendingIter->first < ackBasedId) && (sendingIter != sendingCache.end())) {
	sendingCache.erase(sendingIter->first);
	sendingIter++;
    }
    return true;
}

/*packetId 一般都是从Pace队列中取出来的，然后根据这个id作为key在sendingCache缓存中查找并发送出去*/
bool BoeSender::sendCallback(const uint32_t packetId, const int retrans, const size_t size) {
    std::map<uint64_t, BoeSegmentMessage *>::iterator cacheIter = sendingCache.find(packetId);
    if (cacheIter == sendingCache.end()) {
	return false;
    }
    if (scc == NULL)
	return false;

    uint64_t nowTs = BaseTimer::getCurrentTime();

    BoeSegmentMessage *segmentMsg = cacheIter->second;
    /*  
     *  填充sendInterval参数  
     *      发送耗费时间 = 当前时间 - 置入缓冲区耗费时间 - 相对起始时间 
     */
    segmentMsg->sendInterval = nowTs - firstTs - segmentMsg->interval;
    /*
     *  生成发送侧唯一ID值，标识每次发送的SegmentMessage的transportSeq ID值
     */
    segmentMsg->transportSeq = transportSeqSeed ++;
    /*
     *  将即将发送的报文加入到history队列中,以提供给FEEDBACK报文使用
     */
    scc->onSendCallback(segmentMsg->transportSeq, segmentMsg->dataSize + SEGMENT_HEADER_SIZE);     
    /* 
     * 这里不需要回收SegmentMessage占用的内存块，在Segment ACK的时候回收即可
     */
    if (!session->sendSegmentMessage(segmentMsg))
	return false;

    return true;
}

/*处理feedback消息*/
bool BoeSender::procFeedbackMsg(BoeFeedbackMessage *feedbackMessage) {
    if (feedbackMessage == NULL)
	return false;

    if (scc ==  NULL)
	return false;

    scc->processFeedback(feedbackMessage);
    return true;
}
