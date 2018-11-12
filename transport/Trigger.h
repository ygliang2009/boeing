#ifndef __TRIGGER_H_
#define __TRIGGER_H_
class Trigger {
public:
    virtual bool networkChangeTrigger(uint32_t bitrate, uint8_t fractionLoss, uint32_t rtt) = 0;

public:
    Trigger() {}
    virtual ~Trigger() {}
};

#endif
