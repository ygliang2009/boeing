#ifndef __BITRATE_CONTROLLER_H_
#define __BITRATE_CONTROLLER_H_

#include "SenderBitrateEstimation.h"
#include "Internal.h"
#include "transport/SendCongestionCtrl.h"

#define NORMAL_NOTIFY_TIMER	2000


class BoeSendCongestionCtrl; 
/*
 * 带宽调整的主要类，这里有两个入口会触发调用底层调整带宽的方法：onReport方法和procBitrateHeartbeat方法
 * 其中，onReport方法在发送端根据接收端的Feedback包检测丢包情况lossInfoMsg后调用onReport方法
 * procHeartbeat方法由发送端每5ms调用Sender的heartBeat方法，进而调用procBitrateHeartbeat方法 这两个方法
 * 都会通知上层码率发生变化
 *
 *   ** 那么，onReport和procBitrateHeartbeat方法有什么区别呢 **
 *   onReport方法只有在网络拥塞且有丢包时才会调用，而网络延迟不拥塞，WEBRTC是不会立即调用上层通知网络发生变化的
 *   在网络不丢包，只有延迟的情况时，WEBRTC用procBitrateHeartbeat做上层通知保证
 */
class BitrateController {
public:
    SenderBitrateEstimation *est;

    BoeSendCongestionCtrl *scc;
    
    int64_t lastBitrateUpdateTs;
    int64_t notifyTs;
    uint32_t reservedBitrateBps;
    uint32_t lastBitrateBps;
    uint8_t lastFractionLoss;
    uint32_t lastRtt;
    uint32_t lastReservedBitrateBps;

public:
    BitrateController(BoeSendCongestionCtrl *);
    ~BitrateController();
    
    bool setStartBitrate(const uint32_t &startBitrate);
    bool procBitrateHeartbeat(const int64_t &, const uint32_t &);

    bool setBitrates(const uint32_t &, const uint32_t &, const uint32_t &);
    bool resetBitrates(const uint32_t &, const uint32_t &, const uint32_t &);

    bool onLossInfoResult(const uint32_t &, const int64_t &, \
        const uint8_t &, const int &, const uint32_t &);

    bool onDelayBasedResult(const int &, const int &, const uint32_t &, const int &);

    int getParameter(uint32_t &, uint8_t &, uint32_t &);

private:
    bool __maybeTriggerNetworkChanged();
};

#endif
