#ifndef __NOTIFY_TYPE_H_
#define __NOTIFY_TYPE_H_
#include <stdint.h>
typedef uint32_t NotifyType ;

enum {
    /*停止播放通知 */
    STOP_PLAY_NOTIFY = 0x01,
    /*网络中断通知 */
    NET_INTERRUPT_NOTIFY,
    /*句柄断连通知*/
    DISCONNECT_NOTIFY,
};


#endif
