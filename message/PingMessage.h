#ifndef __PING_MSG_H_
#define __PING_MSG_H_
#include "Header.h"
#include "Message.h"

class BoePingMessage: public BoeMessage {
public:
    uint32_t ts;

public:
    BoePingMessage() {};
    ~BoePingMessage() {}; 
    bool Process(const char*, const BoeHeader*);
    bool Build(char*) const;
};
#endif
