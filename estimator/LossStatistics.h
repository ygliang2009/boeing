#ifndef __LOSS_STATISTICS_H_
#define __LOSS_STATISTICS_H_
#include <stdint.h>

#define LOSS_STATISTICS_WINDOW_MS	500

class LossStatistics {
public:
    /*上一次统计的时间戳*/
    int64_t statTs;
    /*上一次统计的最大序号*/
    int64_t prevMaxId;
    /*接收到的最大序号*/
    int64_t maxId;
    /*每次report作为一个周期所接收到的报文个数,calculate时清空，incoming做累加*/
    int count;

public:
    LossStatistics();
    ~LossStatistics();

    bool calculate(const int64_t nowTs, uint8_t* fractionLoss, int* num);
    bool incoming(const uint16_t seq);
};

#endif
