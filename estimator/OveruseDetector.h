#ifndef __OVERUSE_DETECTOR_H_
#define __OVERUSE_DETECTOR_H_
#include <stdint.h>
#include "EstimatorCommon.h"
#include <math.h>
#include "util/common.h"

#define MIN_NUM_DELTAS			60
#define MAX_TIME_DELTAS_MS		100
#define MAX_ADAPT_OFFSET_MS		15

class OveruseDetector {
public:
    double up;
    double down;
    double overusingTimeThreshold;
    double threshold;
    double timeOverUsing;
    double prevOffset;
    /*threshold更新时间*/
    int64_t updateTs;
    int overuseCounter;
    /*网络负载状态*/
    int state;

public:
    OveruseDetector();
    ~OveruseDetector();

public:
    int overuseDetect(const double, const double, const int, const int64_t);

private:
    void __updateThreshold(const double modifiedOffset, const int64_t nowTs);
};

#endif
