#include "SenderHistory.h"

SenderHistory::SenderHistory() {

}

SenderHistory::~SenderHistory() {
    std::map<uint16_t, FeedbackPacket *>::iterator it = historyBuffer.begin();
    for (; it != historyBuffer.end(); it++) {
	CLEANUP(it->second);	
    }
}

bool SenderHistory::historyAdd(FeedbackPacket *feedbackPacket) {
    std::map<uint16_t, FeedbackPacket *>::iterator historyIter = historyBuffer.begin();
    /*遍历buffer，删除过期数据*/
    for (; historyIter != historyBuffer.end(); historyIter++) {
	if ((historyIter->second->createTs + limitedMs) < feedbackPacket->sendTs) {
	    if (historyIter->first > lastAckSeqNum) {
		bufferedBytes = BOE_MAX(\
		    bufferedBytes - historyIter->second->payloadSize, 0);
	 	
		lastAckSeqNum = historyIter->first;
	    }
            
            CLEANUP(historyIter->second);
	    historyBuffer.erase(historyIter->first);
        } else {
	    break;
        }
    }
    /*添加新数据*/
    historyBuffer[feedbackPacket->sequenceNumber] = feedbackPacket;
    bufferedBytes += feedbackPacket->payloadSize;
    if (lastAckSeqNum == 0) {
	lastAckSeqNum = BOE_MAX(lastAckSeqNum, feedbackPacket->sequenceNumber - 1);
    }
    return true;
}

int SenderHistory::historyGet(const uint16_t seq, FeedbackPacket *feedbackPacket, int removeFlag) {
    std::map<uint16_t, FeedbackPacket *>::iterator iter;

    for (int64_t i = lastAckSeqNum; i < feedbackPacket->sequenceNumber; i++) {
        iter = historyBuffer.find(i);
	if (iter != historyBuffer.end()) {
	    bufferedBytes -= iter->second->payloadSize;
	    BOE_MAX(bufferedBytes, 0);
        }
    }
    lastAckSeqNum = BOE_MAX(feedbackPacket->sequenceNumber, lastAckSeqNum);
    iter = historyBuffer.find(feedbackPacket->sequenceNumber);

    if (iter == historyBuffer.end())
	return false;

    feedbackPacket = iter->second;
    /*查找完毕报文是否移除该报文信息*/
    if (removeFlag == 1) {
	CLEANUP(iter->second);
	historyBuffer.erase(iter->first);
    }
    return true;
}
