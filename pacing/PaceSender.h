#ifndef __SEND_PACE_CONTROLLER_
#define __SEND_PACE_CONTROLLER_
#include <stdint.h>
#include <stdlib.h>
#include <AlrDetector.h>
#include <IntervalBudget.h>
#include <PaceQueue.h>
#include "util/BaseTimer.h"
#include "util/common.h"
#include "Sender.h"

/*发包最小间隔*/
#define MIN_PACKET_LIMIT_MS		5
/*发包最大时间差, 长时间不发送报文一次性发送出去很多数据会造成网络拥塞*/
#define MAX_INTERVAL_MS			50

#define DEFAULT_PACE_FACTOR		1.5

class BoeSender;

class PaceSender {

public:
    PaceSender();
    ~PaceSender();

    bool setEstimateBitrate(const uint32_t);
    bool setBitrateLimits(const uint32_t);
    int insertPacket(const uint32_t, const int, const size_t, const int64_t);
    int64_t getQueueMs() const;
    size_t getQueueSize() const;
    int64_t expectedQueueMs() const;
    int64_t getLimitedStartTime() const;

    bool paceSend(PacketEvent *);
    bool sendPacket(uint32_t, int, size_t);
    bool tryTransmit(const int64_t nowTs);

    bool setSender(BoeSender *);
    uint64_t getLimitedStartTime();

public:
    uint32_t minSenderBitrateKbps;
    uint32_t estimatedBitrate;
    uint32_t pacingBitrateKbps;
    int64_t lastUpdateTs;
    int64_t firstSentTs;
    AlrDetector *alr;    
    PaceQueue que;
    IntervalBudget mediaBudget;

    BoeSender *sender;
};
#endif
