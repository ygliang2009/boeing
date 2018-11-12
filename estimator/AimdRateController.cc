#include "AimdRateController.h"

AimdRateController::AimdRateController(const uint32_t maxrate, const uint32_t minrate) {
    maxBitrate = maxrate;
    minBitrate = minrate;

    currentBitrate = 0;
    avgMaxBitrateKbps = -1.0f;
    varMaxBitrateKbps = 0.4f;

    aimdState = BRC_HOLD;
    region = BRC_MAXUNKNOWN;

    beta = 0.85f;

    timeLastBitrateChange = -1;
    timeFirstEstimate = -1;

    inited = -1;
    rtt = DEFAULT_RTT;
}

AimdRateController::~AimdRateController() {

}

bool AimdRateController::setRtt(const uint32_t rt) {
    rtt = rt;
    return true;
}

bool AimdRateController::setStartBitrate(const uint32_t bitrate) {
    currentBitrate = bitrate;
    inited = 0;
    return true;
}

bool AimdRateController::setMaxBitrate(const uint32_t maxbitrate) {
    maxBitrate = maxbitrate;
    currentBitrate = BOE_MIN(currentBitrate, maxBitrate);
    return true;
}

bool AimdRateController::setMinBitrate(const uint32_t minbitrate) {
    minBitrate = minbitrate;
    currentBitrate = BOE_MAX(currentBitrate, minBitrate);
    return true;
} 

bool AimdRateController::setEstimateBitrate(const int bitrate, const int64_t nowTs) {
    inited = 0;
    currentBitrate = __clampBitrate(bitrate, bitrate);
    timeLastBitrateChange = nowTs;
    return true;
}

/*带宽区间保护*/
uint32_t AimdRateController::__clampBitrate(uint32_t newBitrate, const uint32_t comingBitrate) {
    const uint32_t maxBitrateBps = 3 * comingBitrate / 2 + 10000;

    if (newBitrate > currentBitrate && newBitrate > maxBitrateBps) {
	newBitrate = BOE_MAX(currentBitrate, maxBitrateBps);
    }

    return BOE_MIN(BOE_MAX(newBitrate, minBitrate), maxBitrate);
}

/*根据当前的rtt值判断是否可以进行带宽调节*/
bool AimdRateController::rateControlPermit(const int64_t nowTs, const uint32_t incomingRate) {
    int64_t reduceInterval = BOE_MAX(BOE_MIN(200, rtt), 10);

    /*如果超过了可调节带宽时间，则返回true*/
    if (nowTs - timeLastBitrateChange >= reduceInterval) {
	return true;
    }
    /*如果是非起始阶段，并且预估的码率比当前码率的一半还要小，则返回true*/
    if (inited == 0 && currentBitrate / 2 > incomingRate) {
	return true;
    }
    return false;
   
}

/*输入预测的带宽，进行aimd带宽调整*/
uint32_t AimdRateController::updateCurrentBitrate(const uint32_t ackedBitrate, const int state, const int64_t nowTs) {
    if (inited == -1) {
	if (timeFirstEstimate < 0) {
	    /*确定第一次update的时间戳*/
	    if (ackedBitrate > 0)
	    	timeFirstEstimate = nowTs;
        }   
	else if ((nowTs - timeFirstEstimate) > INITIALIZATION_TS && (ackedBitrate > 0)) {
		/*5s后进行将统计到的带宽作为初始化带宽*/
		currentBitrate = ackedBitrate;
		inited = 0;
	}
    }
    currentBitrate = __changeBitrate(currentBitrate, state, ackedBitrate, nowTs); 
    
    return currentBitrate;
}

uint32_t AimdRateController::__changeBitrate(\
    uint32_t newBitrate, const int state, const uint32_t ackedBitrate, const int64_t nowTs) {
   
    float ackedKbitrate, maxKbitrate;
 
    if (inited == -1 && state == BW_OVERUSING)
	return currentBitrate;
    /*根据input的状态修改aimdState的取值*/
    __changeState(state, ackedBitrate, nowTs); 
	
    /*预估出来的的码率，kb为单位*/
    ackedKbitrate = ackedBitrate / 1000.0f;
    maxKbitrate = sqrt(avgMaxBitrateKbps * varMaxBitrateKbps);

    switch (aimdState) {
    case BRC_HOLD:
	break;

    case BRC_INCREASE:
	/*avgMaxBitrateKbps是ackedBitrate多次累计的结果*/
	if (avgMaxBitrateKbps >= 0 && ackedKbitrate > avgMaxBitrateKbps + 3 * maxKbitrate) {
	    __changeMaxBandwidthRegion(BRC_MAXUNKNOWN);
	    avgMaxBitrateKbps = -1.0f;
        }
	/*如果带宽接近最大值，则加数曾，如果远不到最大值，则进行倍数增*/
   	if (region == BRC_NEARMAX)
	    newBitrate += __additiveRateIncrease(nowTs, timeLastBitrateChange);
    	else 
	    newBitrate += __multiRateIncrease(nowTs, timeLastBitrateChange, newBitrate);

	timeLastBitrateChange = nowTs;
	break;

    case BRC_DECREASE:
	newBitrate = (uint32_t)(beta * ackedBitrate + 0.5f);
        /*
         * 如果预估的码率不大于当前码率，则以预估码率的0.85倍递减
         * 如果大于当前码率，则以avgMaxBitrateKbps的0.85倍递减
         */	
 	if (newBitrate > currentBitrate) {
	    if (region != BRC_MAXUNKNOWN) {
		/*以0.85倍速率递减 beta = 0.85*/
		newBitrate = (uint32_t)(avgMaxBitrateKbps * 1000 * beta + 0.5f);
	    }
	    newBitrate = BOE_MAX(newBitrate, currentBitrate);
	}
	/*如果带宽下降了，就在这里标识当前带宽已接近最大值*/
	__changeMaxBandwidthRegion(BRC_NEARMAX);

	/*如果预估出来的带宽，仍然比平均值小很多，则把avgMaxBitrateKbps设置到一个很小的状态*/
	if (ackedKbitrate < avgMaxBitrateKbps - 3 * maxKbitrate)
	    avgMaxBitrateKbps = -1.0f;
 	
	inited = 0;
	/*更新avgMaxBitrateKbps*/
	__updateMaxBitrateEstimate(ackedKbitrate);

	aimdState = BRC_HOLD;
	timeLastBitrateChange = nowTs;

    default:
	break;
    }
 
    return __clampBitrate(newBitrate, ackedBitrate);
}

bool AimdRateController::__changeState(const int state, const uint32_t ackedBitrate, const int64_t nowTs) {
    switch (state) {
    /*
     * 需要连续两次Normal状态才能达到Increase状态
     */
    case BW_NORMAL:
	if (aimdState == BRC_HOLD) {
	    timeLastBitrateChange = nowTs;
   	    aimdState = BRC_INCREASE;
        }
	break;

    case BW_OVERUSING:
	/*如果预估出来的带宽就是overusing，则啥都别说，直接置网络状态为decrease*/
	if (aimdState != BRC_DECREASE)
	    aimdState = BRC_DECREASE;
	break;

    case BW_UNDERUSING:
	if (aimdState == BRC_HOLD)
	    break;

    default:
        break;
    }
    return true;
}

bool AimdRateController::__changeMaxBandwidthRegion(const int r) {
    region = r;
    return true;
}

/*当前预估的带宽只影响到avgMaxBitrateKbps的0.05*/
bool AimdRateController::__updateMaxBitrateEstimate(const float incomingBitrateKbps) {
    const float alpha = 0.05f;
    if (avgMaxBitrateKbps == -1.0f)
	avgMaxBitrateKbps = incomingBitrateKbps;
    else 
	avgMaxBitrateKbps = (1 - alpha) * avgMaxBitrateKbps + alpha * incomingBitrateKbps;
    /*下面这些看不懂啥意思，照搬过来*/
    const float norm = BOE_MAX(avgMaxBitrateKbps, 1.0f);
    /*
     * 预估maxBitrate 变量并且归一化avgMaxBitrate
     * (Estimate the max bit rate variance and normalize the variance with the average max bit rate)
     */
    avgMaxBitrateKbps = (1 - alpha) * varMaxBitrateKbps + \
	alpha * (avgMaxBitrateKbps - incomingBitrateKbps) * (\
	    avgMaxBitrateKbps - incomingBitrateKbps) / norm;
    /* 0.4 ~ 14 kbit/s at 500 kbit/s */
    if (varMaxBitrateKbps < 0.4f) 
	varMaxBitrateKbps = 0.4f;
    
    /* 2.5f ~ 35 kbit/s at 500 kbit/s */
    if (varMaxBitrateKbps > 2.5f)
	varMaxBitrateKbps = 2.5f;

    return true;
}

uint32_t AimdRateController::__additiveRateIncrease(const int64_t nowTs, const int64_t lastTs) {
    return (uint32_t)(nowTs - lastTs) * __getNearMaxIncRate() / 1000;
}

uint32_t AimdRateController::__multiRateIncrease(\
    const int64_t nowTs, const int64_t lastTs, const uint32_t currentBitrate) {

    double alpha = 1.08;
    uint32_t tsSince;

    if (lastTs > -1) {
	tsSince = BOE_MIN((uint32_t)(nowTs - lastTs), 1000);
	alpha = pow(alpha, tsSince / 1000.0);
    }
    return (uint32_t)(BOE_MAX(currentBitrate * (alpha - 1.0), 1000.0));
}

int AimdRateController::__getNearMaxIncRate() const {
    double bitsPerFrame = currentBitrate / 30.0;
    /* 1000 是 MTU ??? */
    double packetsPerFrame = ceil(bitsPerFrame / (8.0 * 1000.0));
    double avgPacketSizeBits = bitsPerFrame / packetsPerFrame;

    /*Approximate over use estimate delay to 100 ms*/
    const int64_t responseTime = (rtt + 100) * 2;
    /*1s内一个包的bits数目/响应时间 */
    return  (int)(BOE_MAX(4000, (avgPacketSizeBits * 1000)/ responseTime));
}


