#ifndef __CONNECT_ACK_MESSAGE_H_
#define __CONNECT_ACK_MESSAGE_H_

#include "Message.h"
#include "Header.h"

class BoeConnectAckMessage:public BoeMessage {
public:
    /*CONNECT消息中的cid部分*/
    uint32_t cid;
    /*ACK结果*/
    uint32_t result;

public:
    BoeConnectAckMessage() {};
    ~BoeConnectAckMessage() {};
    bool Process(const char *, const BoeHeader *);
    bool Build(char *) const;
};

#endif
