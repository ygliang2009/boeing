#include "Receiver.h"
#include "util/common.h"
#include "util/BaseTimer.h"

FrameCache::FrameCache(){
}

FrameCache::~FrameCache() {
    free(frames);
}

BoeFrame::BoeFrame() {
    segments = NULL;
}

BoeFrame::~BoeFrame() {

}

BoeReceiver::BoeReceiver() {
    baseSeq = 0;
    baseUid = 0;
}

BoeReceiver::~BoeReceiver() {
    baseSeq = 0;
    baseUid = 0;
    delete cache;
}

bool BoeReceiver::heartBeat() {
    return true;
}

void BoeReceiver::setBaseUid(const uint64_t baseUserId) {
     baseUid = baseUserId;  
}

bool BoeReceiver::putMessage(const BoeSegmentMessage *segMsg) {
    if (rcc != NULL) {
	rcc->onReceived(segMsg->transportSeq, segMsg->interval + segMsg->sendInterval, segMsg->dataSize + SEGMENT_HEADER_SIZE);
    }

    uint32_t seq = segMsg->packetId;
    uint64_t segTs = segMsg->interval;
    if (actived == 0 ||seq <= baseSeq)
        return false;
    
    if (maxSeq == 0 && segMsg->packetId > segMsg->index) {
	maxSeq = segMsg->packetId - segMsg->index - 1;
        baseSeq = segMsg->packetId - segMsg->index -1;
    	maxTs = segTs;
    }

    //根据接收到的segment，更新lossMap中的报文信息
    if (!this->__updateLossMap(seq, segTs)){
	return false;
    }
    if (!this->__updateFrameCache(segMsg)) {
	return false;
    } 
    if (segMsg->ftype == 1) { 
        //关键帧
	firState = FIR_NORMAL;
    }
    if (seq == baseSeq + 1) {
	baseSeq = seq;
    } 
     
    maxSeq = BOE_MAX(maxSeq, seq);
    maxTs = BOE_MAX(maxTs, segMsg->interval);
    return true;
}

bool BoeReceiver::onReceived() {
    return true;
}

bool BoeReceiver::createComponent() {
    if (session == NULL)
        return false;
    if (!__createFrameCache())
        return false;
    if (!__createCongestionCtrl())
	return false;

    return true;
}

bool BoeReceiver::__createFrameCache() {
    if (session == NULL) {
        return false;
    }
    cache = new FrameCache();
    cache->waitTimer = session->rtt + 2 * session->rttVar;
    cache->state = BUFFER_WAITING;
    cache->minSeq = 0;
    //默认的帧间隔为100ms
    cache->frameTimer = 100;
    cache->f = 1.0f; 
    cache->size = CACHE_SIZE;
    cache->frames = new BoeFrame[CACHE_SIZE]();
    return true;
}

bool BoeReceiver::__createCongestionCtrl() {
    rcc = new BoeRecvCongestionCtrl();
    rcc->minBitrate = MIN_BITRATE;
    rcc->maxBitrate = MAX_BITRATE;
    return true;
}

bool BoeReceiver::__updateLossMap(const uint32_t seq, const uint32_t segTs) {
    uint64_t nowTs = BaseTimer::getCurrentTime();
    uint32_t i, space;   
 
    if (maxTs + 3000 < segTs) { 
        lossMap.clear();
	//todo receiver_send_fir() 请求对端重发关键帧
    }
    else if (lossMap.size() > 300) 
        lossMap.clear();
	//todo receiver_send_fir() 请求对端重发关键帧
    else {
        if (session->rtt/2 < session->rttVar)
	    space = 0;
        else
	    space = BOE_MIN(100, BOE_MAX(30, session->rtt/2));
        //在lossMap中删除掉已经接收到的报文信息 
        lossMap.erase(seq);

	/*
 	 * (max_seq,seq) 之间的报文都有可能是丢失报文
	 * 把max_seq到seq之间的seq更新到lossMap中
	 */
        for (i = maxSeq; i < seq; i++) {
	    if (lossMap.find(i) == lossMap.end()) {
                LossValue lossValue;
	        lossValue.lossCount = 0;
		lossValue.ts = nowTs - space; 
	        
                lossMap[i] = lossValue; 
            }
        }	
    }
    return false;
}

bool BoeReceiver::__updateFrameCache(const BoeSegmentMessage *segMsg) {
    if (segMsg->total < segMsg->index) {
	return false;
    }

    uint32_t fid = segMsg->fid;
    if (fid < cache->minFid) {
        return false;
    }
    if (fid > cache->maxFid) {
	//todo 添加播放缓冲区处理机制
    }

    BoeFrame *frame = &(cache->frames[fid]);
    frame->fid = fid;
    frame->ts = segMsg->interval;
    frame->frameType = segMsg->ftype;

    if (frame->segments == NULL || frame->segNumber == 0) {
	frame->segments = new BoeSegmentMessage[segMsg->total]();
        frame->segNumber = segMsg->total; 
    }
    if (!frame->segments[INDEX(segMsg->index)].isNull()) 
        frame->segments[INDEX(segMsg->index)] = *segMsg;
    return true;
}

bool BoeReceiver::ackSegmentMessage(char *sendMsg, const uint32_t seq) {
    if (sendMsg == NULL)
	return false;
    
    int32_t maxCount = -1;

    uint64_t curTs = BaseTimer::getCurrentTime();

    if (ackTs + ACK_REAL_TIME < curTs) {
        BoeHeader header;
        BoeSegmentAckMessage segAckMsg;

        char *p = sendMsg;
        header.uid = session->uid;
        header.mid = MSG_SEGMENT_ACK;
	header.headerEncode(sendMsg);
        p += header.header_size;
       
        segAckMsg.basePacketId = baseSeq; 
        segAckMsg.ackedPacketId = seq;
        segAckMsg.nackNum = 0;
	
        std::map<LossKey, LossValue>::iterator it = lossMap.begin();
	uint32_t spaceFactor;
     
        for (; it != lossMap.end(); it++) {
	    uint32_t lossSeq = it->first;
	    //lossMap中的报文号小于连续接收到的报文号，则放弃
	    if (lossSeq <= baseSeq) 
		continue;
       	   
            LossValue *lossValue = &it->second;
	    spaceFactor = (BOE_MIN(1.3, 1 + (lossValue->lossCount * 0.1)) * (session->rtt + session->rttVar));

	    if (lossValue->ts + spaceFactor < curTs && lossValue->lossCount < 15 && segAckMsg.nackNum < NACK_NUM) {
	    	segAckMsg.nack[segAckMsg.nackNum++] = lossSeq - baseSeq;
	        lossValue->ts = curTs;
	        lossValue->lossCount++; 
		lossCount++;
	     } 
      
	    if (lossCount > maxCount)
	        maxCount = lossCount; 
        }	      
        if (!segAckMsg.Build(p)) {
            return false;
        }
    }
    //更新cache的wait_timer
    if (!__updateCacheWaitTimer(maxCount)) {
	return false;
    }
    return true;
}

bool BoeReceiver::procRecvHeartbeat(const uint64_t nowTs) {
    if (session == NULL)
	return false;

    if (!__updateCacheWaitTimer(0))
        return false;
    //每隔1s更新一次缓冲区等待时间cache->wait_time 
    if (cacheTs + BOE_MAX(session->rtt + session->rttVar, 1000) < nowTs) {
	if (lossCount == 0) { 
	    cache->waitTimer = BOE_MAX(cache->waitTimer * 7/8, cache->frameTimer);
	}
        else if (cache->waitTimer > 2 * (session->rttVar + session->rtt)) {
	    //缓冲区时间太长，则适当缩短缓冲区时间
	    cache->waitTimer = BOE_MAX(cache->waitTimer * 15/16, cache->frameTimer);
	    cacheTs = BaseTimer::getCurrentTime();
	    lossCount = 0;
        }
    }
    if (rcc != NULL) {
        char *feedbackMsg = (char *)malloc(sizeof(char) * MTU);
        memset(feedbackMsg, 0, MTU);

	if (!rcc->procRccHeartbeat(feedbackMsg)){
	    return false;
  	}
        session->getConn()->sendPacket(feedbackMsg);
    }
    return true;  
}

bool BoeReceiver::updateRtt(const uint32_t rtt) {
    /*在Sender里已经设置了RTT值，这里不做设置*/
    return true;
}

//定期更新cache的播放缓存
bool BoeReceiver::__updateCacheWaitTimer(const uint32_t lossMaxCount) {
    uint32_t delay;

    if (lossMaxCount > 0) {
        delay = (lossMaxCount + 8) * (session->rtt + session->rttVar)/8;
    } else {
        delay = session->rtt + session->rttVar;
    }
    cache->waitTimer = (cache->waitTimer * 7 + delay)/8;
    //接收缓冲区等待时间要大于发送缓冲区的帧间隔
    cache->waitTimer = BOE_MAX(cache->waitTimer, cache->frameTimer); 
    return true;
}

bool BoeReceiver::setActive(const uint32_t uid) {
    if (actived == 1)
        return false;

    actived = 1;
    baseUid = uid;
    activeTs = BaseTimer::getCurrentTime();
    firState = FIR_NORMAL;
    cache->frameTimer = 50;
    return true;
}

bool BoeReceiver::onBitrateChange(const uint32_t targetBitrate) {
    if (rcc != NULL)
        return rcc->onBitrateChange(targetBitrate);

    return true;
}
