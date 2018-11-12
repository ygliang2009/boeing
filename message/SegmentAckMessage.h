#ifndef __SEGMENT_ACK_MESSAGE_H_
#define __SEGMENT_ACK_MESSAGE_H_

#include "Message.h"

class BoeSegmentAckMessage:public BoeMessage {
public:
    /*被接收端确认连续最大的包ID  在构造SEG_ACK报文时被赋值为recv->base_seq*/
    uint32_t basePacketId; 
    /*立即确认的报文序号id，用于计算rtt*/
    uint32_t ackedPacketId;
    /*重传序列*/
    uint8_t nackNum;
    uint16_t nack[NACK_NUM];

public:
    BoeSegmentAckMessage() {};
    ~BoeSegmentAckMessage() {};
    bool Process(const char *, const BoeHeader *);
    bool Build(char *) const;
};

#endif
