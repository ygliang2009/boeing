#include "BitrateController.h"

BitrateController::BitrateController(BoeSendCongestionCtrl *sendCongestionCtrl) {
    est = new SenderBitrateEstimation(\
        DEFAULT_BITRATECTRL_MINBITRATE, DEFAULT_BITRATECTRL_MAXBITRATE);

    lastBitrateUpdateTs = BaseTimer::getCurrentTime();
    notifyTs = lastBitrateUpdateTs;

    if (scc == NULL)
	scc = sendCongestionCtrl;
}


BitrateController::~BitrateController() {
    CLEANUP(est);
}


bool BitrateController::setStartBitrate(const uint32_t startBitrate) {
    /*将预估出来的码率赋值给est->currBitrate*/
    if (!est->setSendBitrate(startBitrate)) {
	return false;
    }

    __maybeTriggerNetworkChanged();
    return true;
} 


/*每到一个心跳时间，则对前面做的带宽预估值作调整，并通知上一层决策*/
bool BitrateController::procBitrateHeartbeat(const int64_t nowTs, const uint32_t ackedBitrate) {

    uint32_t bitrate = 0, rtt = 0;
    uint8_t fractionLoss = 0;

    if (nowTs < lastBitrateUpdateTs + 100)
	return false;
    
    est->updateEstimation(nowTs, ackedBitrate);
    if (scc != NULL) {
        if (getParameter(&bitrate, &fractionLoss, &rtt) == 0) {
	    /*网络状态发生变化，则进行通知*/
	    scc->onNetworkChange(bitrate, fractionLoss, rtt);
	    notifyTs = nowTs;
 	} else if (notifyTs + NORMAL_NOTIFY_TIMER <= nowTs) {
	    /*每2s触发一次，通知上层进行FEC和丢包重传策略调整*/
	    scc->onNetworkChange(bitrate, fractionLoss, rtt);
	    notifyTs = nowTs;
	}	
    }
    return true;
}


bool BitrateController::__maybeTriggerNetworkChanged() {
    uint32_t rtt, bitrate;    
    uint8_t fractionLoss;

    if (scc != NULL && getParameter(&bitrate, &fractionLoss, &rtt) == 0) {
        if(!scc->onNetworkChange(bitrate, fractionLoss, rtt)) {
            return false;
        }	
    }
    notifyTs = BaseTimer::getCurrentTime();
    return true;
}


bool BitrateController::setBitrates(\
    const uint32_t bitrate, const uint32_t minBitrate, const uint32_t maxBitrate) {

    if (!est->setSendBitrate(bitrate))
        return false;

    if (!est->setMinBitrate(minBitrate))
	return false;

    if (!est->setMaxBitrate(maxBitrate))
	return false;

    __maybeTriggerNetworkChanged(); 
    return true;
}


bool BitrateController::resetBitrates(\
    const uint32_t bitrate, const uint32_t minBitrate, const uint32_t maxBitrate) {

    if (est != NULL)
	delete est;

    est = new SenderBitrateEstimation(\
        DEFAULT_BITRATECTRL_MINBITRATE, DEFAULT_BITRATECTRL_MAXBITRATE);

    if (!est->setMinBitrate(minBitrate))
        return false;
    if (!est->setMaxBitrate(maxBitrate))
	return false;
    if (!est->setSendBitrate(bitrate))
	return false;
    
    __maybeTriggerNetworkChanged(); 
    return true;
}


bool BitrateController::onLossInfoResult(\
    const uint32_t rtt, const int64_t currTs, const uint8_t fractionLoss, \
        const int packetsNum, const uint32_t ackedBitrate) {

    if (packetsNum <= 0)
        return false;
    est->updateReceiverBlock(fractionLoss, rtt, packetsNum, currTs, ackedBitrate);
    __maybeTriggerNetworkChanged();
    return true;
}


bool BitrateController::onDelayBasedResult(\
    const int update, const int probe, const uint32_t targetBitrate, const int state) {
    /*bitrate没有更新*/
    if (update == -1)
        return false;
    if (est == NULL)
        return false;

    est->updateDelayBase(BaseTimer::getCurrentTime(), targetBitrate, state);
    __maybeTriggerNetworkChanged();
    return true;
}

/*从est中获取参数bitrate, fractionLoss, rtt几个参数*/
int BitrateController::getParameter(uint32_t* bitrate, uint8_t* fractionLoss, uint32_t* rtt) {
    int ret = -1;
    uint32_t currBitrate;

    currBitrate = est->currBitrate;
    currBitrate -= BOE_MIN(currBitrate, reservedBitrateBps);

    *rtt = est->lastRtt;
    *fractionLoss = est->lastFractionLoss;
    *bitrate = BOE_MAX(currBitrate, est->minConfBitrate);
  
    if (*fractionLoss != lastFractionLoss || *rtt != lastRtt || *bitrate != lastBitrateBps || lastReservedBitrateBps != reservedBitrateBps)  {
	lastFractionLoss = *fractionLoss;
	lastBitrateBps = *bitrate;
   	lastRtt	= *rtt;
	lastReservedBitrateBps = reservedBitrateBps;
	ret = 0;
    }
    return ret;
}
