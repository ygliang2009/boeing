#include "RecvCongestionCtrl.h"
#include "util/BaseTimer.h"

BoeRecvCongestionCtrl::BoeRecvCongestionCtrl() {
    estimateProxy = new RecvEstimateProxy;
}

BoeRecvCongestionCtrl::~BoeRecvCongestionCtrl() {
    delete estimateProxy;
}

bool BoeRecvCongestionCtrl::sendCallback() {
    return true;
}

bool BoeRecvCongestionCtrl::onReceived(const uint16_t seq, const uint32_t timestamp, const size_t size) {
    int64_t nowTs = BaseTimer::getCurrentTime();
    if (!estimateProxy->incoming(nowTs, 0, seq))
        return false;
    if (!lossStat.incoming(seq))
        return false;
    return true;
}

bool BoeRecvCongestionCtrl::procRccHeartbeat(char *respFeedbackMsg) {
    int64_t nowTs = BaseTimer::getCurrentTime();
    uint8_t fractionLoss;
    int num;

    if (estimateProxy == NULL)
	return false;
    /*构造Feedback报文,5ms一次*/
    BoeFeedbackMessage feedbackMsg;
    /*Feedback Message 包括丢包信息和延迟信息，都有频次控制*/
    if(lossStat.calculate(nowTs, &fractionLoss, &num)) {
    	feedbackMsg.flag |= LOSS_INFO_MSG; 
	feedbackMsg.fractionLoss= fractionLoss;
	feedbackMsg.packetNum = num;
    }
    
    if(estimateProxy->procProxyHeartbeat(&feedbackMsg)) {
        feedbackMsg.flag |= PROXY_TS_MSG;
    }

    if (feedbackMsg.flag != 0) {
        feedbackMsg.Build(respFeedbackMsg);
    }
    return true;
}

bool BoeRecvCongestionCtrl::onBitrateChange(const uint32_t targetBitrate) {
    if (estimateProxy ==  NULL)
        return false;
    return estimateProxy->onBitrateChange(targetBitrate); 
}
