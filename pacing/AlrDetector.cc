#include "AlrDetector.h"

AlrDetector::AlrDetector():intervalBudget(0, 0) {
    alrStartTimestamp = -1;
}

AlrDetector::~AlrDetector() {

}

/*
 * 上层的paceTryTransmit方法每隔一段时间尝试调用这个方法
 * 因此这个算法先对调用者做bytesRemain- 操作，然后计算出elapseMs的这段时间增加了多少可以发送的字节
 */
bool AlrDetector::bytesSent(const size_t bytes, const int64_t deltaTimestamp) {
    int percent;
    if (!intervalBudget.useBudget(bytes))
        return false;
    if (!intervalBudget.increaseBudget(deltaTimestamp)) 
        return false;

    percent = intervalBudget.budgetLevelPercent();
    /*进行alrStartedTimestamp进行判定*/
    if (percent > ALR_START_BUDGET_PERCENT && alrStartTimestamp == -1) {
	/*发送budget还有很多空闲*/
	alrStartTimestamp = BaseTimer::getCurrentTime();
    } else if (percent < ALR_STOP_BUDGET_PERCENT) {
	alrStartTimestamp = -1;
    }
    return true;
}

bool AlrDetector::setBitrate(const int bitrateBps) {
    int targetBitrateKbps;
    targetBitrateKbps = bitrateBps * ALR_BANDWIDTH_USAGE_PERCENT / (1000 * 100);
    intervalBudget.setTargetRateKbps(targetBitrateKbps);
    return true;
}

int64_t AlrDetector::getAppLimitedStartedTs() const {
    return alrStartTimestamp;
}
