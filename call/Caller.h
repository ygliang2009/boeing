#ifndef __CALL_H_
#define __CALL_H_

struct TargetTransferRate {
    uint32_t atTime;
    uint32_t targetRate;
    int64_t lossRateRatio;
    int64_t roundTripTime;  
};

class Caller {
public:
    virtual bool onTargetTransferRate(TargetTransferRate) = 0;

public:
    Caller() {}
    virtual ~Caller() {}
};

#endif
