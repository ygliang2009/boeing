#ifndef __MESSAGE_PROCESSOR_
#define __MESSAGE_PROCESSOR_
#include "MessageType.h"
#include "Session.h"

class BoeMsgProcessor {
public:
    static bool procConnectMsg(const char *, const BoeHeader *, char *);
    static bool procSegmentMsg(Session*, char*, BoeHeader*, char*);
};


#endif
