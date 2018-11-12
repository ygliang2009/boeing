#ifndef __ESTIMATOR_COMMON_H_
#define __ESTIMATOR_COMMON_H_
/*网络连通状况*/
enum {
    BW_NORMAL,
    /*
     * 这个状态往往发生在刚刚发生网络拥塞，并且当前正在恢复中
     * 基本现象表现为变化斜率为负值，但是如果再合理范围内，代码中表示为大于-detector.threshold
     * 则属于正常恢复范畴，标记state = NORMAL。如果小于-detector.threshold，表示刚刚的网络达到
     * 一个峰值，而当前处于迅速下降期间，这时，标记state = UNDERUSING，预估算法直接返回
     */
    BW_UNDERUSING,
    BW_OVERUSING
};
/*Aimd调节状态*/
enum {
    BRC_HOLD,
    BRC_INCREASE,
    BRC_DECREASE
};
/*带宽拥塞状态*/
enum {
    BRC_NEARMAX,
    BRC_ABOVEMAX,
    BRC_MAXUNKNOWN
};

#define DEFAULT_RTT		200

#endif
