#include "RecvEstimateProxy.h"
#include "util/common.h"

RecvEstimateProxy::RecvEstimateProxy() {
    headerSize = DEFAULT_PACKET_SIZE;
    ssrc = 0;
    hbTs = -1;
    wndStartSeq = 0;
    maxArrivalSeq = 0; 
    sendIntervalMs = DEFAULT_PROXY_INTERVAL_TIME;
}

RecvEstimateProxy::~RecvEstimateProxy() {

}

//arrival_ts: seg包到达时间
//ssrc: 0 ????
//seq: seg包的sequence num

/*
 * 整合arrival_ts结构中的到达包信息，删除过期的包，添加新到达的包
 */
bool RecvEstimateProxy::incoming(const uint64_t arrivalTs, const uint32_t ssrc_, const uint16_t seq) {
    ssrc = ssrc_;

    if (seq > wndStartSeq + 32767)
        return false;
    if (maxArrivalSeq <= wndStartSeq) {
	int num = 0;
        std::map<uint64_t, uint64_t>::iterator iter = arrivalTimes.begin();
        for (; iter != arrivalTimes.end(); iter++) {
            if (iter->first < seq && arrivalTs >= iter->second + BACK_WINDOWS_MS && num < MAX_IDS_NUM) {
                arrivalTimes.erase(iter++); 
		num++;
 	    }
        }
    }
    if (wndStartSeq == 0)
	wndStartSeq = seq;
    else if (seq < wndStartSeq)
	wndStartSeq = seq;
   
    maxArrivalSeq = BOE_MAX(maxArrivalSeq, seq);
    arrivalTimes[seq] = arrivalTs;     
    return true;
}

bool RecvEstimateProxy::heartbeat(int64_t cur_ts, BoeFeedbackMessage *feedbackMsg) {
    return true;
}

bool RecvEstimateProxy::bitrateChanged(const uint32_t bitrate) {
    return true;
}

bool RecvEstimateProxy::__buildFeedbackMsg(BoeFeedbackMessage *feedbackMsg) {
    if (feedbackMsg == NULL)
	return false;
    if (maxArrivalSeq <= wndStartSeq && arrivalTimes.size() > 2)
        return false;

    feedbackMsg->minTs = -1;
    feedbackMsg->samplesNum = 0;
    feedbackMsg->baseSeq = wndStartSeq; 
    
    std::map<uint64_t, uint64_t>::iterator it = arrivalTimes.begin();
    uint64_t newStartSeq;
 
    for (; it != arrivalTimes.end(); it++) {
	if (it->first >= wndStartSeq) {
	    feedbackMsg->samples[feedbackMsg->samplesNum++].seq = it->first;
	    feedbackMsg->samples[feedbackMsg->samplesNum++].ts = it->second;
        }
	//更新下一个feedback的起始位置	
	newStartSeq = it->first + 1;
        if (feedbackMsg->samplesNum >= MAX_FEEDBACK_COUNT)
	    break;
    }
    if (feedbackMsg->samplesNum > 2) {
	wndStartSeq = newStartSeq;
    }
    return true;
}

bool RecvEstimateProxy::procProxyHeartbeat(BoeFeedbackMessage *feedbackMsg) {
    int64_t nowTs = BaseTimer::getCurrentTime();
    if ((nowTs - hbTs) < sendIntervalMs) {
        return false;	
    }
    return __buildFeedbackMsg(feedbackMsg); 
}
