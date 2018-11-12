#ifndef __ACK_BITRATE_ESTIMATOR_
#define __ACK_BITRATE_ESTIMATOR_
#include "FeedbackAdapter.h"
#include "util/common.h"

#define RATE_WND_MS		150
#define INITIAL_RATE_WND_MS	500

/*通过接收端反馈的FEEDBACK报文，粗略地预估有效码率*/
class AckBitrateEstimator {
public:
    AckBitrateEstimator();
    ~AckBitrateEstimator();

public:
    int64_t alrEndedTs;
    int64_t currWinMs;
    int64_t prevTs;
  
    int sum;
    float bitrateEstimate;
    float bitrateEstimateVar;

public:
    bool incomingArrivalPackets(FeedbackPacket[], int);
    uint32_t ackEstimatorBitrateBps() const;
    bool setAlrEnded(const int64_t);

private:
    bool __maybeExpectFastChange(int64_t);
    float __updateWindow(const int64_t, const size_t, const int);
    bool __update(const int64_t, const size_t);
};
#endif
