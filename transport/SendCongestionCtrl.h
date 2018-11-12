#ifndef __SEND_CONGESTION_CTRL_H_
#define __SEND_CONGESTION_CTRL_H_

#include "estimator/DelayBasedEstimator.h"
#include "estimator/BitrateController.h"
#include "estimator/AckBitrateEstimator.h"
#include "estimator/FeedbackAdapter.h"
#include "pacing/PaceSender.h"
#include "util/BaseTimer.h"
#include "transport/Trigger.h"
#include "Internal.h"

#define MAX_QUEUE_MS		250

class BoeSender;

class BitrateController;

class PaceSender;

class BoeSendCongestionCtrl {

public:
    BoeSendCongestionCtrl();
    ~BoeSendCongestionCtrl();

    bool procSccHeartbeat();
    bool onNetworkChange(uint32_t, uint8_t, uint32_t);

    bool registerTrigger(Trigger *trigger);
    bool updateRtt(uint32_t);
    bool addPacket(const uint32_t, const int, const size_t);

    bool setSender(BoeSender *);
    bool onSendCallback(uint32_t, size_t);
    bool processFeedback(BoeFeedbackMessage *);

    bool setBaseBitrate();
public:
    /*视频报文在发送queue的最大延迟*/
    int acceptQueueMs;
    int wasInAlr;
    int32_t rtt;

public:
    /*基于延迟的带宽预估模型*/
    DelayBasedEstimator *delayBasedEstimator;
    /*码率控制器， 会根据bwe，ack rate和loss进行综合码率调节*/
    BitrateController *bitrateController;
    /*远端确认收到的数据带宽评估器*/
    AckBitrateEstimator *ackBitrateEstimator;
    /*发送端步长控制器*/
    PaceSender *paceSender;
    /*处理反馈信息的适配器*/
    FeedbackAdapter *feedbackAdapter;

    Trigger *trigger;
    /*透传给PaceSender，作为PaceSender的回调*/
    BoeSender *sender;
};

#endif
