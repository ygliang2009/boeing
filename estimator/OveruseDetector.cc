#include "OveruseDetector.h"

OveruseDetector::OveruseDetector() {
    up = 0.0187;
    down = 0.039;
    overusingTimeThreshold = 12.5;
    threshold = 12.5;
    updateTs = -1;
    timeOverUsing = -1;
    state = BW_NORMAL; 
}

OveruseDetector::~OveruseDetector() {
    
}

int OveruseDetector::overuseDetect(\
    const double offset, const double tsDelta, const int numOfDeltas, const int64_t nowTs) {

    double T;
    if (numOfDeltas < 2)
        return BW_NORMAL;
    
    T = BOE_MIN(numOfDeltas, MIN_NUM_DELTAS) * offset;
    /*网络延迟增量逐渐扩大*/
    if (T > threshold) {
	if (timeOverUsing == -1)
	    timeOverUsing = tsDelta / 2;
        else 
	    timeOverUsing = tsDelta;

        overuseCounter++;

        if (timeOverUsing > overusingTimeThreshold && overuseCounter > 1) {
	    if (offset > prevOffset) {
		timeOverUsing = 0;
		overuseCounter = 0;
		state = BW_OVERUSING;
	    }
    	}
    } 
    else if (T < -threshold) {
    /*网络延迟增量逐步减小，需要加大贷款码率*/
	timeOverUsing = -1;
	overuseCounter = 0;
	state = BW_UNDERUSING;
    } 
    else {
	timeOverUsing = -1;
	overuseCounter = 0;
	state = BW_NORMAL;
    }

    prevOffset = offset;
    __updateThreshold(T, nowTs);

    return state;
}


/*根据一系列规则更新threshold值*/
void OveruseDetector::__updateThreshold(const double modifiedOffset, const int64_t nowTs) {
    double k;
    int64_t timeDelta;

    if (updateTs == -1)
	updateTs = nowTs;
    
    if (fabs(modifiedOffset) > threshold + MAX_ADAPT_OFFSET_MS) {
	updateTs = nowTs;
	return;
    }

    k = fabs(modifiedOffset) < threshold ? down : up; 
    timeDelta = BOE_MIN(nowTs - updateTs, MAX_TIME_DELTAS_MS);

    threshold += k * (fabs(modifiedOffset) - threshold) * timeDelta;
    threshold = BOE_MAX(6.0f, BOE_MIN(600.0f, threshold))  ;

    updateTs = nowTs; 
}
