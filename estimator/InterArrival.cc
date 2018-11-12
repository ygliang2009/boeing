#include "InterArrival.h"

InterArrival::InterArrival(const int bst, const uint32_t groupTicks) {
     burst = bst;
     timeGroupLenTicks = groupTicks;
     resetGroupTs();
}


InterArrival::~InterArrival() {

}

int InterArrival::__inOrder(uint32_t ts) {
    if (curTsGroup.completeTs == -1)
        return 0;
    if (curTsGroup.firstTs <= ts)
	return 0;

    return -1;
}

int InterArrival::__belongsToBurst(uint32_t ts, int64_t arrivalTs) {
    int64_t arrivalTsDelta;
    uint32_t tsDelta;
    int proDelta;

    if (burst == -1)
        return false;

    arrivalTsDelta = arrivalTs - curTsGroup.completeTs;
    tsDelta = ts - curTsGroup.timestamp;
    if (tsDelta == 0)
	return true;

    proDelta = (int)(arrivalTsDelta - tsDelta);
    if (proDelta < 0 && arrivalTsDelta <= BURST_THRESHOLD_MS)
	return true;

    return false;
}

bool InterArrival::__newGroup(uint32_t ts, int64_t arrivalTs) {
    uint32_t diff;

    if (curTsGroup.completeTs == -1)
	return false;
    else if (__belongsToBurst(ts, arrivalTs) == 0)
	return false;
    else {
	diff = ts - curTsGroup.firstTs;
	return (diff - timeGroupLenTicks) ? true : false;
    }

}

bool InterArrival::resetGroupTs() {
    curTsGroup.size = 0;
    curTsGroup.firstTs = 0;
    curTsGroup.timestamp = 0;
    curTsGroup.completeTs = -1;
    curTsGroup.lastSysTs = 0;

    prevTsGroup.size = 0;
    prevTsGroup.firstTs = 0; 
    prevTsGroup.timestamp = 0;
    prevTsGroup.completeTs = -1;
    prevTsGroup.lastSysTs = 0;

    return true;
}



bool InterArrival::computeDeltas(\
    const uint64_t timestamp, const uint64_t arrivalTs, const uint64_t nowTs, \
        const size_t payloadSize, uint32_t *timestampDelta, int64_t *arrivalDelta, \
            int64_t *sizeDelta) {

    int ret = false;
    int64_t arrDelta, sysDelta;
    uint32_t tsDelta;
    
    if (curTsGroup.completeTs == -1) {
        curTsGroup.timestamp = timestamp;
	curTsGroup.firstTs = timestamp;
    } else if (__inOrder(timestamp) == -1) {
	return ret;
    } else if (__newGroup(timestamp, arrivalTs) == 0) {
	if (prevTsGroup.completeTs >= 0) {
	    tsDelta = curTsGroup.timestamp - prevTsGroup.timestamp;
	    arrDelta = curTsGroup.completeTs - prevTsGroup.completeTs;
            sysDelta = curTsGroup.lastSysTs - prevTsGroup.lastSysTs;

            if (arrDelta > sysDelta + OFFSET_THRESHOLD_MS) {
		resetGroupTs();
		return ret;
	    } 
	    if (arrDelta < 0) {
		numConsecutive++;
	  	if (numConsecutive > 3)
		    resetGroupTs();

		return ret;
	    } else {
		numConsecutive = 0;
	    }

	    *sizeDelta = curTsGroup.size - prevTsGroup.size;
	    *timestampDelta = tsDelta;
	    *arrivalDelta = arrDelta;
	    ret = true;
        }
	prevTsGroup = curTsGroup;

	curTsGroup.firstTs = timestamp;
	curTsGroup.timestamp = timestamp;
	curTsGroup.size = 0;
    } else {
	curTsGroup.timestamp = BOE_MAX(curTsGroup.timestamp, timestamp);
    }
    curTsGroup.size += payloadSize;
    curTsGroup.completeTs = arrivalTs;
    curTsGroup.lastSysTs = nowTs;
    return true;
}
