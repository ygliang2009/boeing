#ifndef __DISCONNECT_ACK_MESSAGE_H_
#define __DISCONNECT_ACK_MESSAGE_H_

#include "Message.h"

class BoeDisConnectAckMessage:public BoeMessage {
public:
    /*同CONNECT消息的CID*/
    uint32_t cid;
    /*DISCONNECT的结果*/
    uint32_t result;

public:
    BoeDisConnectAckMessage() {};
    ~BoeDisConnectAckMessage() {};
    bool Process(const char *, const BoeHeader *);
    bool Build(char *) const;
};

#endif
