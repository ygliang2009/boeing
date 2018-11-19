#include "TrendlineEstimator.h"

TrendlineEstimator::TrendlineEstimator() {

}

TrendlineEstimator::~TrendlineEstimator() {

}

void TrendlineEstimator::updateTrendline(\
    const double &recvDeltaMs, const double &sendDeltaMs, const int64_t &arrivalTs) {

    double deltaMs = recvDeltaMs - sendDeltaMs;
    DelayHistory *history;
    numOfDeltas ++;
    if (numOfDeltas > TRENDLINE_MAX_COUNT) 
	numOfDeltas = TRENDLINE_MAX_COUNT;
 
    if (firstArrivalTs == -1) 
	firstArrivalTs = arrivalTs;
   
    accDelay += deltaMs;
    smoothedDelay = smoothingCoef * smoothedDelay + (1 - smoothingCoef) * accDelay;
   
    history = &que[index++ % windowSize];
    history->arrivalDelta = (double)(arrivalTs - firstArrivalTs);
    history->smoothedDelay = smoothedDelay;
    
    if (index > windowSize) {
	trendline = __linearFitSlope();
    }
}

double TrendlineEstimator::__linearFitSlope() const {
    uint64_t i;
    double sumX, sumY, avgX, avgY, numerator, denominator;

    sumX = sumY = avgX = avgY = 0;
    numerator = 0;
    denominator = 0;

    for (i = 0; i < windowSize; i++) {
	sumX += que[i].arrivalDelta;
	sumY += que[i].smoothedDelay;
    }
    avgX = sumX / windowSize;
    avgY = sumY / windowSize;
    
    for (i = 0; i < windowSize; i++) {
	numerator += (que[i].arrivalDelta - avgX) * (que[i].smoothedDelay - avgY);
	denominator += (que[i].arrivalDelta - avgX) * (que[i].arrivalDelta - avgX);
    }
    if (denominator != 0)
	return numerator / denominator;
    else
	return 0;
}

double TrendlineEstimator::trendlineSlope() const {
    return thresholdGain * trendline;
}
