#include "IntervalBudget.h"

IntervalBudget::IntervalBudget (const int targetKbps, const int buildupUnderuse) {
    bytesRemaining = 0;
    canBuildupUnderuse = buildupUnderuse;
    setTargetRateKbps(targetKbps);
}

IntervalBudget::~IntervalBudget() {

}

bool IntervalBudget::setTargetRateKbps(const int targetKbps) {
    targetRateKbps = targetKbps;
    maxBytesInBudget = INTERVAL_WINDOW_MS * targetRateKbps;
    bytesRemaining = BOE_MIN(BOE_MAX(-maxBytesInBudget, bytesRemaining), maxBytesInBudget);
    return true;
}

bool IntervalBudget::increaseBudget(const int deltaTimestamp) {
    int bytes = targetRateKbps * deltaTimestamp / 8;
    if (bytesRemaining < 0 || canBuildupUnderuse == 0) 
        bytesRemaining = BOE_MIN(bytesRemaining + bytes, maxBytesInBudget);
    else 
	bytesRemaining = BOE_MIN(bytes, maxBytesInBudget);

    return true;
} 

bool IntervalBudget::useBudget(const int bytes) {
    bytesRemaining -= bytes;
    bytesRemaining = BOE_MAX(-maxBytesInBudget, bytesRemaining);
    return true;
}

bool IntervalBudget::budgetLevelPercent() const {
    return bytesRemaining * 100 / maxBytesInBudget;
}

size_t IntervalBudget::budgetRemaining() const{
    return bytesRemaining > 0 ? bytesRemaining : 0;
}
