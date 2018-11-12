#ifndef __INTER_ARRIVAL_H_
#define __INTER_ARRIVAL_H_
#include <stdlib.h>
#include <stdint.h>
#include "util/common.h"

#define BURST_THRESHOLD_MS	50
#define OFFSET_THRESHOLD_MS	3000

/*
 * 一组网络报文的收发时间戳，这个间隔统计算法是一直了webRTC中的GCC，这里时间戳都是毫秒级计算
 */
class TimestampGroup {
public:
    size_t size;
    uint32_t firstTs;
    uint32_t timestamp;
    int64_t completeTs;
    int64_t lastSysTs;
};

class InterArrival{
public:
    TimestampGroup curTsGroup;
    TimestampGroup prevTsGroup;
    int numConsecutive;
    int burst;
    /*一个Group的最大时间范围，默认是5ms*/
    uint32_t timeGroupLenTicks;

public:
    InterArrival(const int, const uint32_t);
    ~InterArrival();

public:
    bool computeDeltas(\
     const uint64_t, const uint64_t, const uint64_t, \
         const size_t, uint32_t*, int64_t*, int64_t*);

    bool resetGroupTs();

private:
    int __inOrder(const uint32_t);
    int __belongsToBurst(const uint32_t, const int64_t);
    bool __newGroup(const uint32_t, const int64_t);
};
#endif
