#ifndef __PONG_MSG_H_
#define __PONG_MSG_H_
#include "Header.h"
#include "Message.h"

class BoePongMessage: public BoeMessage {
public:
    uint32_t ts;
public:
    BoePongMessage() {};
    ~BoePongMessage() {}; 
    bool Process(const char*, const BoeHeader*);
    bool Build(char*) const;
};
#endif
