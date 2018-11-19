#ifndef __LOOP_H_
#define __LOOP_H_
#include "Session.h"
#include "stdint.h"
#include "message/Message.h"
#include "message/MessageType.h"
#include "message/MessageProcessor.h"
#include <stdio.h>
#include <errno.h>
#include "Header.h"


class LoopEvent {
private:
    int64_t loop_interval;

public:
    Session session;


public:
    bool run();
    bool init();

private:
    bool __processMessage(char*, BoeHeader *);
    bool __processHeartbeat(const int64_t &);
};

#endif
