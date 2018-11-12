#include "SenderBitrateEstimation.h"

SenderBitrateEstimation::SenderBitrateEstimation(\
    const uint32_t minBitrate, const uint32_t maxBitrate) {

    minConfBitrate = minBitrate;
    maxConfBitrate = maxBitrate;
    hasDecreasedSinceLastFractionLoss = -1;
    lastFeedbackTs = -1;
    lastPacketReportTs = -1;
    lastTimeoutTs = -1;
    firstReportTs = -1;

    lowLossThreshold = LOW_LOSS_THRESHOLD;
    highLossThreshold = HIGH_LOSS_THRESHOLD;
    bitrateThreshold = 1000 * BITRATE_THRESHOLD_KBPS;

    beginIndex = 0;
    endIndex = 0;
    prevFractionLoss = -1;
    state = BW_NORMAL;
}

SenderBitrateEstimation::~SenderBitrateEstimation() {

}

bool SenderBitrateEstimation::setSendBitrate(const uint32_t sendBitrate) {
    if(!__capBitrateToThreshold(BaseTimer::getCurrentTime(), sendBitrate)) {
        return false;
    }
    /*清空历史最小记录*/
    endIndex = beginIndex = 0;
    return true;
}


bool SenderBitrateEstimation::setBitrates(\
    const uint32_t sendBitrate, const uint32_t minBitrate, const uint32_t maxBitrate) {

    if (!setMinBitrate(minBitrate))
        return false;
    if (!setMaxBitrate(maxBitrate))
        return false;
    if (!setSendBitrate(sendBitrate))
	return false;
    return true;
}

/*基于延迟的码率预估完成后，会调用这个函数，bitrate为基于延迟的码率预估结果*/
bool SenderBitrateEstimation::updateDelayBase(\
    const int64_t curTs, const uint32_t bitrate, const int st) {
    
    delayBaseBitrate = bitrate;
    state = st;
    __capBitrateToThreshold(curTs, currBitrate);
    return true;
}
/*
 * slope.acc是一个累加值，在达到LOSS_WND_SIZE之前，这个值不断累加
 * 超过LOSS_WND_SIZE以后，每次acc = acc - frag + 本次delta;
 */
double SenderBitrateEstimation::__slopeFilterUpdate(const int delta) {
    slopes.slope  = 255;
    slopes.index ++;
    slopes.acc -= slopes.frags[slopes.index % LOSS_WND_SIZE];
    slopes.frags[slopes.index % LOSS_WND_SIZE] = delta;
    slopes.acc += delta;
    
    if (slopes.index > LOSS_WND_SIZE) 
        slopes.slope = slopes.acc * 1.0f / LOSS_WND_SIZE;
    else
        slopes.slope = slopes.acc * 1.0f / slopes.index;
     
    return slopes.slope; 
}

/*更新接收端汇报的丢失延迟数据*/
bool SenderBitrateEstimation::updateReceiverBlock(\
    const uint8_t fractionLoss, const uint32_t rtt, const int numberOfPackets, \
        const int64_t currTs, const int32_t ackedBitrate)  {

    int lostPacketsQ8;
    
    lastFeedbackTs = currTs;
    if (firstReportTs == -1) 
        firstReportTs = currTs;
    if (rtt > 0) 
        lastRtt = rtt;
    /*numberOfPackets为不丢包情况下的最大报文数*/ 
    if (numberOfPackets > 0) {
	lostPacketsQ8 = fractionLoss * numberOfPackets;
        lostPacketsSinceLastLossUpdateQ8 += lostPacketsQ8;
        expectedPacketsSinceLastLossUpdate += numberOfPackets;

        if (expectedPacketsSinceLastLossUpdate < LIMIT_NUM_PACKETS)
	    return false;
        /*计算丢包率，这里采用大于20个报文的方式作为计算依据*/
        hasDecreasedSinceLastFractionLoss = -1;
        lastFractionLoss = lostPacketsSinceLastLossUpdateQ8 / expectedPacketsSinceLastLossUpdate;
        lostPacketsSinceLastLossUpdateQ8 = 0;
        expectedPacketsSinceLastLossUpdate = 0;
        lastPacketReportTs = currTs;
   
        if (prevFractionLoss != -1)
	    __slopeFilterUpdate(lastFractionLoss - prevFractionLoss);

        prevFractionLoss = lastFractionLoss;
       
        if (!updateEstimation(currTs, ackedBitrate)) {
	    return false;
        }
    }
    return true;
}

bool SenderBitrateEstimation::updateEstimation(\
    const uint32_t currTs, const uint32_t ackedBitrate) {
    /*
     *  在起始的2s时间，如果没有丢包，算法会允许进行起始阶段的码率探测
     *  调节的码率值参考REMB and/or delayBased estimator 
     *  (We trust the REMB and/or delay-based estimate during the first 2 seconds if
     *   haven't had any packet loss reported, to allow startup bitrate probing)
     */
    uint32_t newBitrate = currBitrate, pos;
    int64_t timeSincePacketReportMs, timeSinceFeedbackMs;
    double loss;
    /*如果是开始阶段并且网络没有发生丢包， 取REMB和delayBase中的大者为当前的决策带宽*/ 
    if (lastFractionLoss == 0 && __isInStartPhase(currTs) == 0) {
        newBitrate = BOE_MAX(bweIncoming, newBitrate);
	newBitrate = BOE_MAX(delayBaseBitrate, newBitrate); 
        
        if (newBitrate != currBitrate) {
	    endIndex = beginIndex = 0;
            minBitrates[endIndex].ts = currTs;
            minBitrates[endIndex].bitrate = currBitrate;
	    endIndex ++;
	    __capBitrateToThreshold(currTs, newBitrate);
	    return true;
        }	
    }
    /*更新minBitrates记录*/
    __updateMinHistory(currTs);

    if (lastPacketReportTs == -1) {
	__capBitrateToThreshold(currTs, currBitrate);
	return true;
    }
    /*进行丢包相关的码率控制*/
    timeSincePacketReportMs = currTs - lastPacketReportTs;
    timeSinceFeedbackMs = currTs - lastFeedbackTs;
    /*FEEDBACK_INTERVAL_MS = 5s */
    if (timeSincePacketReportMs * 1.2 < FEEDBACK_INTERVAL_MS) {
	/*丢包率是用0-255表示的，在网络数据包中采用uint8_t表示，所以我们在计算时需要转换成百分数来进行判断*/
        loss = lastFractionLoss / 256.0;
        if (slopes.slope < 0.8f && loss > lowLossThreshold && state != BW_OVERUSING) {
	    /*网络延迟并没有增大，丢包率处于固定范围内，说明是噪声丢包， 进行带宽抢占*/
	    pos = beginIndex % MIN_HISTORY_ARR_SIZE;
	     /*1s以内的最小码率*/
	    newBitrate = (uint32_t)(minBitrates[pos].bitrate * 1.08 + 0.5 + 1000);
        } else {
	    /*网络发送丢包率 < 2%时，进行码率上升*/
	    if (currBitrate < bitrateThreshold || loss < lowLossThreshold) {
		pos = beginIndex % MIN_HISTORY_ARR_SIZE;
	  	/*1s以内的最小码率*/
		newBitrate = (uint32_t)(minBitrates[pos].bitrate * 1.08 + 0.5 + 1000);
  	    } else if (currBitrate > bitrateThreshold) {
		/* 2% < loss < 10% 维持当前码率 */
	        if (loss < highLossThreshold) {
		}
		else {
		/* loss > 10%  进行码率下降，根据丢包率占比进行下降*/
		    if (hasDecreasedSinceLastFractionLoss == -1 && (\
			currTs >= lastDecreaseTs + BWE_DECREASE_INTERVAL_MS + lastRtt)) {
		        hasDecreasedSinceLastFractionLoss = 0;
			newBitrate = (uint32_t )(currBitrate * (512 - lastFractionLoss)/512.0f);
		   	if (ackedBitrate > 0)
			    newBitrate = BOE_MAX(ackedBitrate, newBitrate);		
                    }		
		}	
  	    }
 	}
    }  else if (\
	timeSinceFeedbackMs > FEEDBACK_TIMEOUT_INTERVALS * FEEDBACK_INTERVAL_MS && (\
		lastTimeoutTs == -1 || currTs > lastTimeoutTs + TIMEOUT_INTERVAL_MS)) {
	/*Feedback消息丢失并超时， 进行带宽下降并清空相关丢包统计 */
	newBitrate = newBitrate * 4/5;
	lostPacketsSinceLastLossUpdateQ8 = 0;
	expectedPacketsSinceLastLossUpdate = 0;
	lastTimeoutTs = currTs;
    }

    __capBitrateToThreshold(currTs, newBitrate);

    return true;
}

bool SenderBitrateEstimation::__isInStartPhase(const int64_t currTs) const {
    if (firstReportTs == -1 || currTs < firstReportTs + START_PHASE_MS) {
	return true;
    }
 
    return false;
}

/*
 * 更新最小码率历史记录
 *    minBitrates保存的是1s之内的minBitrates, beginIndex到endIndex按照minBitrate从小到大排列
 */
bool SenderBitrateEstimation::__updateMinHistory(const int32_t currTs) {
    uint32_t i, pos;
    /*删除超过1s间隔的记录*/
    for (i = beginIndex; i < endIndex; ++i) {
	pos = i % MIN_HISTORY_ARR_SIZE;
        if (minBitrates[pos].ts + BWE_INCREASE_INTERVAL_MS < currTs) {
	    beginIndex = i + 1;
	    minBitrates[pos].bitrate = 0;
	    minBitrates[pos].ts = 0;
        } else {
	    break;
        }
    }
     
    pos = (endIndex - 1) % MIN_HISTORY_ARR_SIZE;
    /*minBitrates为只保留1s以内的所有元素，并且是个从小到大排列的数组,比当前大的bitrate元素都被删除掉*/
    while (beginIndex < endIndex && minBitrates[pos].bitrate >= currBitrate)  {
	/*如果最后一个记录的码率大于当前计算出来的带宽，删除。 这样，保证最后一个计算的码率是最大码率*/
        minBitrates[pos].bitrate = 0;
        minBitrates[pos].ts = 0;
	endIndex--;
        pos = (endIndex - 1) % MIN_HISTORY_ARR_SIZE;
    }

    if (endIndex == beginIndex) {
	endIndex = beginIndex = 0;
    }
    pos = endIndex % MIN_HISTORY_ARR_SIZE;
    if (endIndex == beginIndex + MIN_HISTORY_ARR_SIZE)
	beginIndex ++;

    endIndex++;
    minBitrates[pos].bitrate = currBitrate;
    minBitrates[pos].ts = currTs;
    return true;
}

/*基于预估出来的bitrate, delay-based bitrate, bwe-incoming bitrate 取其中的最小值 */
bool SenderBitrateEstimation::__capBitrateToThreshold(\
    const uint64_t currTs, const uint32_t bitrate) {

    uint32_t finalBitrate = bitrate;

    if (bweIncoming > 0 && bitrate > bweIncoming)
	finalBitrate = bweIncoming;
   
    /*取当前预估的bitrate和基于延迟的bitrate两者最小值*/ 
    if (delayBaseBitrate > 0 && finalBitrate > delayBaseBitrate)
	finalBitrate = delayBaseBitrate;
    
    if (finalBitrate > maxConfBitrate)
	finalBitrate = maxConfBitrate;
    
    if (finalBitrate < minConfBitrate)
	finalBitrate = minConfBitrate;

    currBitrate = finalBitrate;
    return true;
}
