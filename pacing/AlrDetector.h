#ifndef __ALR_DETECTOR_H_
#define __ALR_DETECTOR_H_

#include "AlrDetector.h"
#include "IntervalBudget.h"
#include <stdint.h>
#include <stdlib.h>
/*
 * 
 */
#define ALR_START_BUDGET_PERCENT		80
#define ALR_STOP_BUDGET_PERCENT			50
#define ALR_BANDWIDTH_USAGE_PERCENT		60


class AlrDetector {
public:
    IntervalBudget intervalBudget;
    int64_t alrStartTimestamp;

public:
    bool bytesSent(const size_t &, const int64_t &);
    bool setBitrate(const int &);
    int64_t getAppLimitedStartedTs() const;
public:
    AlrDetector();
    ~AlrDetector();
};

#endif
