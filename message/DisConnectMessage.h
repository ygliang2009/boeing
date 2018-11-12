#ifndef __DISCONNECT_MESSAGE_H_
#define __DISCONNECT_MESSAGE_H_

#include "Message.h"

class BoeDisConnectMessage:public BoeMessage {
public:
    /*同CONNECT消息中的CID*/
    uint32_t cid;

public:
    BoeDisConnectMessage() {};
    ~BoeDisConnectMessage() {};
    bool Process(const char *, const BoeHeader *);
    bool Build(char *) const;
};

#endif
