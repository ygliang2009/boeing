#ifndef __RECV_ESTIMATE_PROXY_
#define __RECV_ESTIMATE_PROXY_
#include "message/FeedbackMessage.h"
#include <map>
#include <stdlib.h>
#include "Internal.h"
#include "util/BaseTimer.h"

#define DEFAULT_PROXY_INTERVAL_TIME		100
#define MAX_IDS_NUM				100
#define BACK_WINDOWS_MS				500

#define MIN_SEND_INTERVAL_MS                    50
#define MAX_SEND_INTERVAL_MS                    250

class RecvEstimateProxy {
public:
    /*通信报文头固定大小*/
    size_t headerSize;
    /*心跳时间戳, UNIX绝对时间戳，毫秒为单位*/
    int64_t heartBeatTs;
    /*发送反馈间隔时间， 按照5%的总可用带宽计算间隔时间*/
    int64_t sendIntervalMs;

    uint32_t ssrc;
    
    uint64_t wndStartSeq;
    uint64_t maxArrivalSeq;

    std::map<uint64_t, uint64_t> arrivalTimes;

    uint32_t feedbackSequence;

public:
    RecvEstimateProxy();
    ~RecvEstimateProxy();
    bool incoming(const uint64_t &, const uint32_t &, const uint16_t &);
    
    bool onBitrateChange(const uint32_t &bitrate);
    bool procProxyHeartbeat(BoeFeedbackMessage *feedbackMsg);

private:
    bool __buildFeedbackMsg(BoeFeedbackMessage *feedbackMsg);
};

#endif
