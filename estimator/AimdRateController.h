#ifndef __AIMD_RATE_CONTROLLER_H_
#define __AIMD_RATE_CONTROLLER_H_
#include <stdint.h>
#include "util/common.h"
#include "estimator/EstimatorCommon.h"
#include <math.h>

#define INITIALIZATION_TS	5000	
/*
 * additive increased multiple decreased controller
 */
class AimdRateController {
public:
    uint32_t minBitrate;
    uint32_t maxBitrate;
    uint32_t currentBitrate;

    float avgMaxBitrateKbps;
    float varMaxBitrateKbps; 

    int aimdState;
    int region;

    int64_t timeLastBitrateChange;
    int64_t timeFirstEstimate;

    uint32_t rtt;
    /*没太理解这个参数的意义，大概也是一个控制参数，多半控制网络起始阶段的带宽情况*/    
    int inited;

    float beta;

public:
    AimdRateController(const uint32_t, const uint32_t);
    ~AimdRateController();

public:
    bool setRtt(const uint32_t);
    bool setEstimateBitrate(const int, const int64_t);
    /*上层设置，通常在receiver初始化阶段设置此参数*/
    bool setMaxBitrate(const uint32_t);
    /*上层设置，通常在receiver初始化阶段设置此参数*/
    bool setMinBitrate(const uint32_t);
 
    bool setStartBitrate(const uint32_t);
    /*判断aimd控制器是否可以进行网络带宽调节*/
    bool rateControlPermit(const int64_t, const uint32_t);
    
    uint32_t updateCurrentBitrate(const uint32_t ackedBitrate, const int state, const int64_t nowTs);
private:
    /*控制当前预估的带宽在一定的区间内*/
    uint32_t __clampBitrate(uint32_t, const uint32_t);
    /*aimd带宽调节的核心函数*/
    uint32_t __changeBitrate(uint32_t, const int, const uint32_t, const int64_t);
    /*根据input的状态修改aimdState的取值*/
    bool __changeState(const int, const uint32_t, const int64_t);
    /**/
    bool __changeMaxBandwidthRegion(const int);
    /*根据当前预估出来的码率调整avgMaxBitrateKbps和maxBitrateKbps值*/
    bool __updateMaxBitrateEstimate(const float);
    /*加数增加带宽算法*/
    uint32_t __additiveRateIncrease(const int64_t, const int64_t);
    /*乘性增加带宽算法*/
    uint32_t __multiRateIncrease(const int64_t, const int64_t, const uint32_t);
    /*根据当前带宽和响应时间预估加数增因子*/
    int __getNearMaxIncRate() const;
};
#endif
