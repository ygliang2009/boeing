#ifndef __RECV_CONGESTION_CTRL_
#define __RECV_CONGESTION_CTRL_
#include "estimator/RecvEstimateProxy.h"
#include "transport/Receiver.h"
#include "estimator/LossStatistics.h"
class BoeReceiver;

class BoeRecvCongestionCtrl {
public:
    BoeReceiver *receiver;
    int minBitrate;
    int maxBitrate;

    RecvEstimateProxy *estimateProxy;
    LossStatistics lossStat;  
 
public:
    BoeRecvCongestionCtrl();
    ~BoeRecvCongestionCtrl();
    
    bool sendCallback();
    bool onReceived(const uint16_t, const uint32_t, const size_t);
    bool procRccHeartbeat(char *);

    bool onBitrateChange(const uint32_t);
};

#endif
