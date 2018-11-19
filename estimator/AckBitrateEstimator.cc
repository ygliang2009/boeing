#include "estimator/AckBitrateEstimator.h"

AckBitrateEstimator::AckBitrateEstimator() {
    alrEndedTs = -1;
    currWinMs = 0;
    prevTs = -1;
    sum = 0;
    bitrateEstimate = -1.0f;
    bitrateEstimateVar = 50.0f;    
}

AckBitrateEstimator::~AckBitrateEstimator() {

}

bool AckBitrateEstimator::incomingArrivalPackets(
    FeedbackPacket feedbackPacket[], const int &size) {

    int i = 0;
    for (; i < size; i++) {
	if(feedbackPacket[i].sendTs >= 0) {
	    __maybeExpectFastChange(feedbackPacket[i].sendTs);
	    __update(feedbackPacket[i].arrivalTs, feedbackPacket[i].payloadSize);
        }
    }   
    return false;
}

bool AckBitrateEstimator::__maybeExpectFastChange(const int64_t &packetSendTs) {
    /*
     * 将码率设置到一个变化较大的范围因子，这个和pacer有关
     * 判断条件：报文发送时间 > 当前时间
     */
    if (alrEndedTs >= 0 && packetSendTs > alrEndedTs) {
	bitrateEstimateVar += 200;
        alrEndedTs = -1;
    }
    return true;
}

bool AckBitrateEstimator::__update(int64_t arrivalTs, const size_t &payloadSize) {
    float bitrateSample, sampleUncertainty, sampleVar, predBitrateEstimateVar;
    int rateWindowsMs = RATE_WND_MS;
    
    if (bitrateEstimate < 0) {
	rateWindowsMs = INITIAL_RATE_WND_MS; 
    }
    bitrateSample = __updateWindow(arrivalTs, payloadSize, rateWindowsMs);
    
    if (bitrateSample < 0.0f)
	return false;
    if (bitrateSample < 0.0f) {
	bitrateEstimate = bitrateSample;
    }
    sampleUncertainty = 10.0f * BOE_ABS(bitrateEstimate, bitrateSample)/bitrateEstimate;
    sampleVar = sampleUncertainty * sampleUncertainty;
    predBitrateEstimateVar = bitrateEstimateVar + 5.f;
    bitrateEstimate = (sampleVar * bitrateEstimate + predBitrateEstimateVar * bitrateSample)/(sampleVar + predBitrateEstimateVar);
    bitrateEstimateVar = sampleVar * predBitrateEstimateVar/(sampleVar + predBitrateEstimateVar);
    return true;
}

float AckBitrateEstimator::__updateWindow(int64_t nowTs, const size_t &size, const int &rateWndMs) {
    float bitrateSample;
    if (nowTs < prevTs) {
	prevTs = -1;
        sum = 0;
        currWinMs = 0;
    }
    if (prevTs >= 0) {
	/*
         * 算法维护了一个不断累积的累计值，当这个累计值超过阈值则进行这一阶段的带宽预估
         */
	currWinMs += nowTs - prevTs;
	/*如果这一轮的sample信息来的比较慢，超过一个周期都没有到达，则进行reset操作*/
        if ((nowTs - prevTs) > rateWndMs) {
	    sum = 0;
            currWinMs %= rateWndMs;
        }
    }
    prevTs = nowTs;
    bitrateSample = -1.0f;

    if (currWinMs > rateWndMs) {
	bitrateSample = 8.0f * (sum/rateWndMs);
        currWinMs -= rateWndMs;
 	sum = 0;
    }
    sum += size; 
    return bitrateSample;
}

uint32_t AckBitrateEstimator::ackEstimatorBitrateBps() const {
    if (bitrateEstimate < 0)
	return 0;
    return (uint32_t)(bitrateEstimate * 1000);
}

bool AckBitrateEstimator::setAlrEnded(int64_t &ts) {
    alrEndedTs = ts;
    return true;
}
