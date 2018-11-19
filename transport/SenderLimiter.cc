#include "SenderLimiter.h"

SenderLimiter::SenderLimiter() {
    winSize = LIMITER_BUFFER_SIZE;
    threshold = MAX_LIMITER_RATE * winSize/(1000 * 8);
    buckets = (uint32_t *)malloc(winSize * sizeof(uint32_t));
    index = 0;
    winBytes = 0;
    oldestTs = -1; 
}

SenderLimiter::~SenderLimiter() {
    free(buckets);
}

bool SenderLimiter::limiterUpdate(const size_t &size, const int64_t &nowTs) {
    int offset;
    if (nowTs < oldestTs)
	return false;

    if (oldestTs == -1)
	oldestTs = nowTs;

    offset = nowTs - oldestTs;
    if (offset > 0)
        __limiterRemove(nowTs);
    
    /*设置当前时刻*/
    buckets[offset % winSize] += size;
    winBytes += size;
    return true;
}

/*判断是否可以进行发送报文*/
bool SenderLimiter::limiterTrySend(const size_t &size, const int64_t &nowTs) {
    if (nowTs < oldestTs) 
        return 0;

    if (oldestTs == -1)
        oldestTs = nowTs;

    __limiterRemove(nowTs);
    return (size + winBytes > threshold) ? false : true;
}

bool SenderLimiter::__limiterRemove(const uint64_t &nowTs) {
    int offset = -1, i = 0, idx;
    offset = nowTs - oldestTs;
    if (offset < winSize + index) {
	for (i = index + 1; i <= offset; ++i) {
	    idx = i % winSize;
	    if (winBytes >= buckets[idx])
	        winBytes = buckets[idx];
	    else
		winBytes = 0;

	    buckets[idx] = 0;
        }
    } else {
	for (i = 0; i < winSize; i++) {
	    idx = i % winSize;
	    buckets[idx] = 0;
	}
	winBytes = 0;
    }
    index = offset;
    return true;
}
