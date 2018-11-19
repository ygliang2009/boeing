#include "DelayBasedEstimator.h"

DelayBasedEstimator::DelayBasedEstimator() {
    lastSeenMs = -1;
    firstTs = BaseTimer::getCurrentTime();

    trendlineWindowSize= TRENDLINE_WINDOW_SIZE;
    trendlineSmoothingCoeff = TRENDLINE_SMOOTHING_COEFF;
    trendlineThresholdGain = TRENDLINE_THRESHOLD_GAIN;

    interArrival = new InterArrival(0, GROUP_TIME);
    aimdRateController = new AimdRateController(AIMD_MAX_BITRATE, AIMD_MIN_BITRATE);
    overuseDetector = new OveruseDetector();  
    trendlineEstimator = new TrendlineEstimator();
}


DelayBasedEstimator::~DelayBasedEstimator() {
    CLEANUP(aimdRateController);
    CLEANUP(overuseDetector);
    CLEANUP(interArrival);
    CLEANUP(trendlineEstimator);
}


void DelayBasedEstimator::resetEstimator() {
    CLEANUP(aimdRateController);
    CLEANUP(overuseDetector);
    CLEANUP(interArrival);
    CLEANUP(trendlineEstimator);
    
    interArrival = new InterArrival(0, GROUP_TIME);
    aimdRateController = new AimdRateController(AIMD_MAX_BITRATE, AIMD_MIN_BITRATE);
    overuseDetector = new OveruseDetector();  
    trendlineEstimator = new TrendlineEstimator();
}


bool DelayBasedEstimator::updateRtt(const uint32_t &rtt) {
    if (aimdRateController == NULL)
        return false;

    aimdRateController->setRtt(rtt);
    return true;
}

BandwidthEstResult DelayBasedEstimator::incomingEstimate(\
    FeedbackPacket packets[], const int &packetsNum, uint32_t ackedBitrate, const uint64_t &nowTs) {
    BandwidthEstResult respEstResult;

    if (packetsNum <= 0)
        return respEstResult;

    if (overuseDetector == NULL)   
        return respEstResult;

    int32_t i = 0;
    uint32_t recoveredFromOveruse = -1;
    uint8_t delayFeedback = 0;

    int prevState = overuseDetector->state;

    for (; i < packetsNum; i++) {
        if (packets[i].sendTs < firstTs)
            continue;
    	/*控制变量*/
        delayFeedback = 1;
        /*
         * 根据反馈来的发包和收包间隔计算延迟斜率，过载判断   trendline滤波算法实现 && 过载检测
         */
        if (!__processEstimate(&packets[i], nowTs)) {
	    /*如果当前报文预估处理出现问题，则跳过*/
	    continue;
        } 
        if (prevState == BW_UNDERUSING && overuseDetector->state == BW_NORMAL) 
	    recoveredFromOveruse = 0;
    }

    /*如果这些反馈的报文都超时了，说明网络不好，强制带宽减半并返回*/
    if (delayFeedback == 0) {

	consecutiveDelayedFeedbacks++;
        if (consecutiveDelayedFeedbacks > MAX_FAILED_COUNT)
             return __longFeedbackDelay(packets[packetsNum - 1].arrivalTs); 
    } 
    else {
	consecutiveDelayedFeedbacks++;
        return __maybeUpdate(overuseDetector->state, ackedBitrate, recoveredFromOveruse, nowTs);
    }

    return respEstResult;
}

bool DelayBasedEstimator::__processEstimate(FeedbackPacket *feedbackPacket, const uint64_t &nowTs) {
    if (feedbackPacket->sendTs < firstTs)
        return false;
   
    if (trendlineEstimator == NULL || overuseDetector == NULL)
        return false;

    uint64_t sendInterval = feedbackPacket->sendTs - firstTs;
    uint32_t tsDelta = 0;
    int64_t tDelta = 0, sizeDelta = 0;    

    /* computeDeltas 和 updateTrendline都属于trendline滤波的算法实现 */
    if(!interArrival->computeDeltas(\
        sendInterval, feedbackPacket->arrivalTs, \
	    nowTs, feedbackPacket->payloadSize, &tsDelta, &tDelta, &sizeDelta)) {
	return false;
    }
    /*计算斜率*/ 
    trendlineEstimator->updateTrendline(tDelta, tsDelta, feedbackPacket->arrivalTs);
    /*
     * 根据斜率变化，过载检测 BW_OVERUSING | BW_UNDERUSING | BW_NORMAL 
     * 
     *     将计算的结果状态保存到overuseDetector->state中
     */
    overuseDetector->overuseDetect(\
	trendlineEstimator->trendlineSlope(), tsDelta, \
	    trendlineEstimator->numOfDeltas, feedbackPacket->arrivalTs);
 
    return true;
}

BandwidthEstResult DelayBasedEstimator::__longFeedbackDelay(const int64_t &arrivalTs) {
    BandwidthEstResult result;
    if (aimdRateController == NULL)
        return result;

    aimdRateController->setEstimateBitrate(aimdRateController->currentBitrate * 1/2, arrivalTs);

    result.updated = 0;
    result.probe = -1;
    result.bitrate = aimdRateController->currentBitrate;
    return result;
}

BandwidthEstResult DelayBasedEstimator::__maybeUpdate(\
    const int &overuseState, const uint32_t &ackedBitrate, \
        const int &recoveredFromOveruse, const int64_t &nowTs) {

    BandwidthEstResult result;
    /*网络过载*/
    if (overuseState == BW_OVERUSING) {
        if (ackedBitrate > 0 && aimdRateController->rateControlPermit(nowTs, ackedBitrate)) {
  	    result.updated = __estimateUpdate(\
		nowTs, ackedBitrate, overuseState, &result.bitrate) == true ? 0 : -1; 
	        	
        } 
	else if (ackedBitrate == 0 \
            && aimdRateController->inited == 0 && aimdRateController->rateControlPermit(\
                nowTs, aimdRateController->currentBitrate * 3/4 - 1) == 0)  {
            /*网络过载且没有检测到acked的带宽，则进行带宽减半*/
	    aimdRateController->setEstimateBitrate(aimdRateController->currentBitrate * 3/4, nowTs);
	    result.updated = 0;
	    result.probe = -1;
	    result.bitrate = aimdRateController->currentBitrate;
        }
        else {
	    /*未过载，判断是否加大码率*/
  	    result.updated = __estimateUpdate(\
    		nowTs, ackedBitrate, overuseState, &result.bitrate) == true ? 0 : -1; 
	    result.recoveredFromOveruse = recoveredFromOveruse; 
	    
        }
    } 
    return result;
}

bool DelayBasedEstimator::__estimateUpdate(\
    const int64_t &nowTs, const uint32_t &ackedBitrate, \
        const int &overuseState, uint32_t *targetBitrate) {

    uint32_t prevBitrate = aimdRateController->currentBitrate;
    *targetBitrate = aimdRateController->updateCurrentBitrate(ackedBitrate, overuseState, nowTs);

    if (aimdRateController->inited == 0 && prevBitrate == *targetBitrate) {
	return true;
    }
    return false;
}
