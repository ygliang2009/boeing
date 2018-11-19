#ifndef __DELAY_BASED_ESTIMATOR_
#define __DELAY_BASED_ESTIMATOR_
#include <stdlib.h>
#include <stdint.h>
#include "InterArrival.h"
#include "AimdRateController.h"
#include "TrendlineEstimator.h"
#include "OveruseDetector.h"
#include "util/BaseTimer.h"
#include "Internal.h"
#include "estimator/FeedbackPacket.h" 

#define TRENDLINE_SMOOTHING_COEFF	0.9
#define	TRENDLINE_THRESHOLD_GAIN	4.0
#define	TRENDLINE_WINDOW_SIZE		20

#define GROUP_TIME			5
#define MAX_FAILED_COUNT		5

#define AIMD_MIN_BITRATE		64000  		/* 8Kb/s */
#define AIMD_MAX_BITRATE		12800000	/* 1.6MB/s */

#define TIMESTAMP_MS			2000

class BandwidthEstResult {
public:
    BandwidthEstResult() {
	/*预估的码率是否有更新*/
	updated = -1;
	probe = -1;
	bitrate = 0;
        recoveredFromOveruse = -1;
    }

    ~BandwidthEstResult() {
    }

public:
   int updated;
   int probe;
   uint32_t bitrate;
   int recoveredFromOveruse; 
};


/*
 * 发送端基于延迟的码率预估模型实现: 
 * 	通过接收端反馈过来的arrival time和发送的send time进行trendline滤波，评估当前码率
 */
class DelayBasedEstimator {
public:
    InterArrival* interArrival;
    /*aimd码率调节*/
    AimdRateController *aimdRateController;
    /*trendline评估模型*/
    TrendlineEstimator *trendlineEstimator;
    /*过载保护模型*/
    OveruseDetector *overuseDetector;

    int64_t lastSeenMs;
    /*起始时间戳 用于计算相对的sender timer时间戳*/
    int64_t firstTs;
    size_t trendlineWindowSize;
    double trendlineSmoothingCoeff;
    double trendlineThresholdGain;
    /*控制参数，反馈的Feedback超时会使这个参数自增，当增到一定阈值的时候，返回拥塞信息*/
    int consecutiveDelayedFeedbacks;
 
public:
    DelayBasedEstimator();
    ~DelayBasedEstimator();

    bool updateRtt(const uint32_t &);

    BandwidthEstResult incomingEstimate(FeedbackPacket[], const int &, uint32_t, const uint64_t &);
    void resetEstimator();


private:
    bool __processEstimate(FeedbackPacket *, const uint64_t &);
    BandwidthEstResult __longFeedbackDelay(const int64_t &);
    BandwidthEstResult __maybeUpdate(const int &, const uint32_t &, const int &, const int64_t &);
    
    bool __estimateUpdate(const int64_t &, const uint32_t &, const int &, uint32_t *);
};

#endif
