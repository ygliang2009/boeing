#include "LossStatistics.h"

LossStatistics::LossStatistics() {
    count = 0;
    maxId = -1;
    prevMaxId = 1;
    statTs = -1;
}

LossStatistics::~LossStatistics() {

}

bool LossStatistics::incoming(const uint16_t &seq) {
    if (prevMaxId == -1 && seq > 0)
        prevMaxId = seq - 1;

    if (maxId < seq)
        maxId = seq;
    ++count;
    return true;
}

bool LossStatistics::calculate(const int64_t &nowTs, uint8_t* fractionLoss, int* num) {
    *fractionLoss = 0;

    int32_t distance = maxId - prevMaxId;
    if (distance <= 0)
	return false;

    if (maxId == -1 || prevMaxId == -1 || ((maxId - prevMaxId) < 20) \
	|| ((nowTs - statTs) < LOSS_STATISTICS_WINDOW_MS)) {

	return false;
    }
    *num = distance;

    if (count >= distance) {
	//表示这一阶段没有丢包
    	*fractionLoss = 0;
    } else {
	*fractionLoss = (distance - count) * 255/ distance;
    }

    prevMaxId = maxId;
    count = 0;
    statTs = nowTs;
    return true;
}
