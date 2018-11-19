#ifndef __SENDER_HISTORY_H_
#define __SENDER_HISTORY_H_
#include <stdlib.h>
#include <stdint.h>
#include <map>
#include "FeedbackAdapter.h"
#include "util/common.h"
#include "Internal.h"

class FeedbackPacket;

class SenderHistory {
public:
    /*buffer的过期时间*/
    uint32_t limitedMs;
    /*最后确认的sequenceNumber*/
    int64_t lastAckSeqNum;
    /*历史数据缓存*/
    std::map<uint16_t, FeedbackPacket *> historyBuffer;
    /*剩余的缓存数据大小*/
    size_t bufferedBytes;

public:
    SenderHistory();
    ~SenderHistory();
    /*添加反馈报文*/
    bool historyAdd(FeedbackPacket *);
    /*消费反馈报文*/
    int historyGet(const uint16_t &, FeedbackPacket*, const int &);
};
#endif
