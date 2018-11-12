#ifndef __SENDER_LIMITER_H_
#define __SENDER_LIMITER_H_

#define MAX_LIMITER_RATE	(1024 * 800)
#define LIMITER_BUFFER_SIZE	300
#include <stdlib.h>
#include <stdint.h>

/*NACK请求带宽限制器*/
class SenderLimiter{
public:
    uint32_t* buckets;
    int32_t index;

    int64_t oldestTs;

    /*统计的时间窗口*/
    int winSize;
    uint32_t winBytes;
    /*最大可发送的请求字节数*/
    uint32_t threshold;

public:
    SenderLimiter();

    ~SenderLimiter();

    bool limiterUpdate(const size_t, const int64_t);

    bool limiterTrySend(const size_t, const int64_t); 

    bool __limiterRemove(const uint64_t);
};

#endif
