#include "SendCongestionCtrl.h"

BoeSendCongestionCtrl::BoeSendCongestionCtrl() {
    wasInAlr = -1;
    acceptQueueMs = MAX_QUEUE_MS;
    rtt = 200;
   
    bitrateController = new BitrateController(this);
    ackBitrateEstimator = new AckBitrateEstimator(); 
    delayBasedEstimator = new DelayBasedEstimator();
}

BoeSendCongestionCtrl::~BoeSendCongestionCtrl() {
    CLEANUP(ackBitrateEstimator);
    CLEANUP(delayBasedEstimator);
    CLEANUP(bitrateController);
    CLEANUP(trigger);
}

bool BoeSendCongestionCtrl::registeTrigger(Trigger *trig) {
    if (trigger == NULL)
        trigger = trig;

    return true;
}

bool BoeSendCongestionCtrl::procSccHeartbeat() {
    int64_t nowTs = BaseTimer::getCurrentTime();
     /*进行Pace发送*/ 
    if (!paceSender->tryTransmit(nowTs))
        return false;
    /*对前一段时间做的预估带宽，进行带宽调节，同时通知上层做相应决策*/
    bitrateController->procBitrateHeartbeat(\
        nowTs, ackBitrateEstimator->ackEstimatorBitrateBps());
    return true;
}

bool BoeSendCongestionCtrl::onNetworkChange(uint32_t bitrate, uint8_t fractionLoss, uint32_t rtt) {
    if (paceSender == NULL || trigger == NULL)
        return false;
    /*把预估出来的码率通知给pacer模块*/
    paceSender->setEstimateBitrate(bitrate);
    /*调用trigger的callback*/
    trigger->networkChangeTrigger(bitrate, fractionLoss, rtt);
    return true;
}

bool BoeSendCongestionCtrl::updateRtt(uint32_t rtt) {
    if (delayBasedEstimator == NULL)
        return false;
    if (!delayBasedEstimator->updateRtt(rtt))
	return false;
    
    return true;
}

bool BoeSendCongestionCtrl::addPacket(const uint32_t packetId, const int retrans, const size_t size) {
    return paceSender->insertPacket(packetId, retrans, size, BaseTimer::getCurrentTime()); 
}

bool BoeSendCongestionCtrl::setSender(BoeSender *snd) {
    sender = snd;
    if (paceSender != NULL)
        paceSender->setSender(snd);
    else
	return false;

    return true;
}

bool BoeSendCongestionCtrl::onSendCallback(uint32_t transSeq, size_t transSize) {
    feedbackAdapter->addPacket(transSeq, transSize);
    return true;
}

bool BoeSendCongestionCtrl::processFeedback(BoeFeedbackMessage *feedbackMessage) {
    if (feedbackMessage == NULL)
        return false;
    if (feedbackAdapter == NULL)
	return false;   
    
    int curAlr = -1;
    uint64_t nowTs = BaseTimer::getCurrentTime();
 
/*
 *      <-- FEEDBACK_MSG

 *
 *      1. 根据feedback的sample信息构造模型，做ackBitrate预估         
 *      (见ackedBitrateEstimator->incmongArrivalPackets函数)
 *
 *      2. 根据trendline滤波模型估算当前网络负载情况，进而进行AIMD码率调节
 *      (见delayBasedEstimator->incomingEstimate函数)
 *
 *      3. 设置BitrateController进而设置SenderEstimationBitrate的delayBaseBitrate参数
 *
 *      4. 进行基于丢包的带宽预估，取新预估的bitrate和delayBaseBitrate的最小值
 *
 *      5. BitrateController和SenderBitrateEstimation是跟上层打交道的函数
 *      如果码率发生变化，会通过这两个函数调用上层callback服务
 *
 *
 */
    /*基于延迟的反馈信息*/ 
    if ((feedbackMessage->flag & PROXY_TS_MSG) == PROXY_TS_MSG) {
        /*将反馈的feedback信息更新到adapter结构中*/
        if (!feedbackAdapter->adapterOnFeedback(feedbackMessage)) {
	    return false;
 	}
        curAlr = paceSender->getLimitedStartTime() > 0 ? 0 : -1;
	if (wasInAlr == 0 && curAlr != 0) {
	    ackBitrateEstimator->setAlrEnded(nowTs); 
        }
	wasInAlr = curAlr;
        /*
         * 根据填充到adapter->packets的延迟信息做第一阶段的码率预估
         */
	ackBitrateEstimator->incomingArrivalPackets(\
	    feedbackAdapter->packets, feedbackAdapter->num);

        BandwidthEstResult estResult = \
	    delayBasedEstimator->incomingEstimate(feedbackAdapter->packets, \
    		feedbackAdapter->num, ackBitrateEstimator->ackEstimatorBitrateBps(), nowTs);
    
        if (estResult.updated == 0) {
            bitrateController->onDelayBasedResult(\
		estResult.updated, estResult.probe, estResult.bitrate, \
		    delayBasedEstimator->overuseDetector->state);
   	}
         
    }
    /*基于丢包的码率预估*/
    if ((feedbackMessage->flag & LOSS_INFO_MSG) == LOSS_INFO_MSG) {
        uint8_t fractionLoss = feedbackMessage->fractionLoss;
        size_t packetNum = feedbackMessage->packetNum;

   	bitrateController->onLossInfoResult(\
	    rtt, nowTs, fractionLoss, packetNum, ackBitrateEstimator->ackEstimatorBitrateBps());    	
    }
    return true;
}

/*
 * 最终会调用到aimd的setMinBitrate和setMaxBitrate等方法
 * todo
 */
bool BoeSendCongestionCtrl::setBaseBitrate() {
   return true;
}
