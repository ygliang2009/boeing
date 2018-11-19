#ifndef __SENDER_ESTIMATION_H_
#define __SENDER_ESTIMATION_H_
#include <stdint.h>
#include <stdlib.h>
#include "EstimatorCommon.h"
#include "util/common.h"
#include "util/BaseTimer.h"

#define LOSS_WND_SIZE			20
#define MIN_HISTORY_ARR_SIZE		128

#define LOW_LOSS_THRESHOLD		0.02f
#define HIGH_LOSS_THRESHOLD		0.1f
#define BITRATE_THRESHOLD_KBPS		0

#define BWE_INCREASE_INTERVAL_MS	1000
#define MAX_BITRATE_BPS			1000000000
#define LIMIT_NUM_PACKETS		20
#define START_PHASE_MS			2000
#define FEEDBACK_INTERVAL_MS		5000
#define FEEDBACK_TIMEOUT_INTERVALS	3
#define TIMEOUT_INTERVAL_MS		1000
#define BWE_DECREASE_INTERVAL_MS	300

class SlopeFilter {
public:
    int frags[LOSS_WND_SIZE];
    int index;
    int acc;
    int count;
    double slope;
};

class MinBitrate {
public:
    int64_t ts;
    uint32_t bitrate;
};

class SenderBitrateEstimation {
public:
    uint32_t currBitrate;
    uint32_t minConfBitrate;
    uint32_t maxConfBitrate;
    int64_t lastFeedbackTs;
    int64_t lastPacketReportTs;
    int64_t lastTimeoutTs;;
    uint32_t lastRtt;
    uint8_t lastFractionLoss;
    
    int prevFractionLoss;

    int8_t hasDecreasedSinceLastFractionLoss;

    uint32_t bweIncoming;
    uint32_t delayBaseBitrate;
    int state;
    int64_t lastDecreaseTs;
    int64_t firstReportTs;
    int initiallyLostPackets;
    
    int lostPacketsSinceLastLossUpdateQ8;
    int expectedPacketsSinceLastLossUpdate;

    float lowLossThreshold;
    float highLossThreshold;
    uint32_t bitrateThreshold;

    SlopeFilter slopes;
    /*维护了一个minBitrate数组，beginIndex到endIndex分别是数组的起始和结束边界*/
    MinBitrate minBitrates[MIN_HISTORY_ARR_SIZE];
    uint32_t beginIndex;
    uint32_t endIndex;
        
public:
    SenderBitrateEstimation(uint32_t, uint32_t);
    ~SenderBitrateEstimation();

    uint32_t getMinBitrate() const {
	return minConfBitrate;
    }

    /*设置最小码率*/
    bool setMinBitrate(const uint32_t &minBitrate) {
	minConfBitrate = BOE_MIN(minConfBitrate, minBitrate);
	return true;
    }

    /*设置最大码率*/
    bool setMaxBitrate(const uint32_t &maxBitrate) {
        if (maxConfBitrate > 0)
	    maxConfBitrate = BOE_MAX(maxConfBitrate, maxBitrate);
    	else 
            maxConfBitrate = MAX_BITRATE_BPS;
	return true;
    }

    /*设置发送码率*/
    bool setSendBitrate(const uint32_t &);

    /*分别设置最小码率，最大码率，发送码率*/
    bool setBitrates(const uint32_t &, const uint32_t &, const uint32_t &);
    /*更新基于延迟的码率控制bitrate参数*/
    bool updateDelayBase(const int64_t &curTs, const uint32_t &bitrate, const int &state);

    bool updateReceiverBlock(\
        const uint8_t &, const uint32_t &, const int &, const int64_t &, const int32_t &);

    bool updateEstimation(const uint32_t &, const uint32_t &);

private:
    double __slopeFilterUpdate(const int &);

    bool __updateMinHistory(const int32_t &);
    
    bool __capBitrateToThreshold(const uint64_t &, const uint32_t &);
    
    bool __isInStartPhase(const int64_t &currTs) const;
};

#endif
