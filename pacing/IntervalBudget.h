#ifndef __INTERVAL_BUDGET_H_
#define __INTERVAL_BUDGET_H_
#include <stdint.h>
#include <stdlib.h>
#include <stdint.h>
#include "util/common.h"
#include "util/BaseTimer.h"

#define INTERVAL_WINDOW_MS	500
#define INTERVAL_DELTA_MS		2000

class IntervalBudget {
public:
    int targetRateKbps;
    int maxBytesInBudget;
    int bytesRemaining;
    int canBuildupUnderuse;

public:
    IntervalBudget(int, int);
    ~IntervalBudget();

    bool setTargetRateKbps(const int &);
    bool increaseBudget(const int &);
    bool useBudget(const int &);
    bool budgetLevelPercent() const;
    size_t budgetRemaining() const;
};

#endif
