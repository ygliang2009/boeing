#ifndef __FEEDBACK_ADAPTER_H_
#define __FEEDBACK_ADAPTER_H_
#include <stdlib.h>
#include <stdint.h>
#include "message/FeedbackMessage.h"
#include "SenderHistory.h"
#include "util/common.h"
#include "util/BaseTimer.h"
#include "Internal.h"
#include <algorithm>
#include "FeedbackPacket.h"

#define FEEDBACK_RTT_WIN_SIZE		32

class SenderHistory;


class FeedbackAdapter {
public:
    FeedbackAdapter();
    ~FeedbackAdapter();

public:
    bool addPacket(const uint64_t, const size_t);
    bool adapterOnFeedback(const BoeFeedbackMessage *);

public:
    SenderHistory *history;
    int32_t minFeedbackRtt;
    int index;
    /*保存feedback sample的rtt信息*/
    int32_t rtts[FEEDBACK_RTT_WIN_SIZE];
    /*packets的长度*/
    int num;
    /*将发送过来的FeedbackSample保存到packets结构中*/
    FeedbackPacket packets[MAX_FEEDBACK_COUNT];

private:
    bool __feedbackQsort();
};
#endif
