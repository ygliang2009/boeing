#ifndef __TRENDLINE_H_
#define __TRENDLINE_H_
#include <stdint.h>
#include <stdlib.h>

#define  TRENDLINE_MAX_COUNT		1000
/*
 * 通过延迟的增长确认过载参数
 */

class DelayHistory {
public:
    double arrivalDelta;
    double smoothedDelay;
};

class TrendlineEstimator {
public:
    size_t windowSize;
    double smoothingCoef;
    double thresholdGain;
    /*好像没有用 todo*/
    uint32_t numOfDeltas;
    int64_t firstArrivalTs;
    double accDelay;
    double smoothedDelay;
    double trendline;
    uint32_t index;
    /*最近的曲线趋势历史点数据*/
    DelayHistory *que;

public:
    TrendlineEstimator();
    ~TrendlineEstimator();

public:
    void updateTrendline(const double, const double, const int64_t);
    double trendlineSlope() const;

private:
     double __linearFitSlope() const; 
};

#endif
