#ifndef __BASETIMER_H_
#define __BASETIMER_H_
#include <stdint.h>
#include <sys/time.h>


class BaseTimer {
public:
    static int64_t getCurrentTime();
};


#endif
