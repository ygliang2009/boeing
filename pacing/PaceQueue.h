#ifndef __PACER_QUEUE_H_
#define __PACER_QUEUE_H_
#include <queue>
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <map>
#include "util/BaseTimer.h"
#define MAX_SPACE_QUEUE_MS		500

class PacketEvent {
public:
    /*通信中的绝对SEQ*/
    uint32_t seq;
    /*是否是重传*/
    int retrans;
    /*报文大小*/
    size_t size;
    /*放入pace queue 的时间戳*/
    int64_t queueTs;
    /*是否已经发送*/
    int sent;

    PacketEvent& operator= (PacketEvent &ev) {
	seq = ev.seq;
        retrans = ev.retrans;
        size = ev.size;
        queueTs = ev.queueTs;
        sent = ev.sent;	
        return *this;
    }
};

class PacketCompare {
public:
    bool operator ()  (PacketEvent* a, PacketEvent* b) const {
        return a->queueTs < b->queueTs ? true : false;
    }
};


class PaceQueue {
public:
    /*pacer可以接受的最大延迟*/
    uint32_t maxQueueMs;

    size_t totalSize;
    /*最早帧的时间戳*/
    int64_t oldestTs;
    /*按照绝对SEQ排队的队列*/
    std::map<uint32_t, PacketEvent* > *cache;
    /*按照时间先后顺序排列的队列*/
    std::priority_queue<PacketEvent*, std::vector<PacketEvent*>, PacketCompare> *l;

public:
    PaceQueue();
    ~PaceQueue();

    uint64_t getOldestTs() const {
       return oldestTs;
    } 

    int pushPacket(PacketEvent *packetEvent);

    PacketEvent* getQueueFront();

    bool queueFinalize();
    
    uint64_t getQueueSize() const {
	return totalSize;
    }

    int queueOldest() const;

    bool queueSent(PacketEvent*);

    bool queueEmpty() const;

    uint32_t targetBitrateKbps(const int64_t &nowTs);
};
#endif
