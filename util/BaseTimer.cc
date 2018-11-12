#include "BaseTimer.h"
#include "string.h"
#include "stdint.h"

int64_t BaseTimer::getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec + (int64_t)tv.tv_sec * 1000 * 1000;
};
