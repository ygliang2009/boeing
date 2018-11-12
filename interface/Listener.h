#ifndef __LISTENER_H_
#define __LISTENER_H_
#include "Session.h"
#include "NotifyType.h"

class Listener {

public:
    virtual bool notifyCallback(Session *, NotifyType notifyType) = 0;
    Listener();
    virtual ~Listener();
};

#endif
