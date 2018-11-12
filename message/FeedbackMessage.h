#ifndef __FEEDBACK_MESSAGE_H_
#define __FEEDBACK_MESSAGE_H_

#include "Message.h"
#include <stdlib.h>

#define MAX_FEEDBACK_COUNT		128


class BoeFeedbackPacket {
public:
    BoeFeedbackPacket();
    ~BoeFeedbackPacket();
    /*创建时间戳*/
    int64_t createTs;
    /*到达时间戳*/
    int64_t arrivalTs;
    /*发送时间戳*/
    int64_t sendTs;
    /*发送通道的报文序号*/
    uint16_t sequenceNumber;
    /*包数据大小*/
    size_t payloadSize;
};

class FeedbackSample {
public:
    uint16_t seq;
    int64_t ts;
};

enum {
    //时延反馈 
    PROXY_TS_MSG = 0x01,
    //丢包反馈
    LOSS_INFO_MSG,
};

class BoeFeedbackMessage:public BoeMessage {
public:
    BoeFeedbackMessage() {};
    ~BoeFeedbackMessage() {};
    bool Process(const char *, const BoeHeader *);
    bool Build(char *) const;

public:
    /*反馈类型掩码*/
    uint8_t flag;
    /*loss info msg*/
    uint8_t fractionLoss;
    int32_t packetNum;

    /*proxy ts msg*/
    int64_t baseSeq;
    int64_t minTs;
    uint8_t samplesNum;

    FeedbackSample samples[MAX_FEEDBACK_COUNT];    
};

#endif
