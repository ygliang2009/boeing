#include "PaceQueue.h"

PaceQueue::PaceQueue() {
    cache = new std::map<uint32_t, PacketEvent* >();
    l = new std::priority_queue<PacketEvent*, std::vector<PacketEvent* >, PacketCompare>();
    maxQueueMs = MAX_SPACE_QUEUE_MS;
}

PaceQueue::~PaceQueue() {
    if (cache != NULL) {
        std::map<uint32_t, PacketEvent* >::iterator it = cache->begin();
        for (; it != cache->end(); it++) {
            /*判断NULL这一步一定要有，因为l和cache数据结构会引用同一个内存块*/
            if (it->second != NULL) {
                /*释放指针占用的内存*/
	        delete it->second;
                it->second = NULL;
            }
        }
        delete cache;
    }
    if (l != NULL) {
        //由于l和cache指向同一个内存区的PacketEvent，所以cache删除了指向的内存，l就不需要重复删除了
        //见insertPacket这一步
        
        /*
	for (int i = 0; i < l->size(); i++) {
            PacketEvent *ev = l->top();
            if (ev != NULL)
 	        delete ev;
	    l->pop();
        }
	*/ 
	delete l;
    }
}

int PaceQueue::pushPacket(PacketEvent *packetEvent) {
    int ret = -1;
    uint32_t key = packetEvent->seq;
    std::map<uint32_t, PacketEvent* >::iterator it = cache->find(key);

    if (it == cache->end())  {
	packetEvent->sent = 0;
        (*cache)[key] = packetEvent;
        /*将发送的packet按照时间顺序压入list*/
        l->push(packetEvent);
	totalSize += packetEvent->size;
	ret = 0;
    }
    if (oldestTs == -1 || oldestTs >  packetEvent->queueTs)
        oldestTs = packetEvent->queueTs;
    return ret;
}

PacketEvent* PaceQueue::getQueueFront() {
    PacketEvent *packetEvent = NULL;

    if (cache->size() == 0)
        return NULL;
    std::map<uint32_t, PacketEvent* >::iterator it = cache->begin();
    while (it != cache->end()) {
        packetEvent = it->second;
        if (packetEvent->sent == 0) 
	    break;
        else
            it++; 
    }
    return packetEvent;
} 

bool PaceQueue::queueFinalize() {
    PacketEvent *packet;
    while (l->size() > 0) {
        packet = l->top();
        if (packet->sent == 1) {
	    l->pop();
            uint32_t key = packet->seq;
            cache->erase(key);
        }
        else
            break;
    }
    /*更新queue状态*/
    if (l->size() == 0) {
	cache->clear();
        totalSize = 0;
        oldestTs = -1;
    } else {
	packet = l->top();
	oldestTs = packet->queueTs;
    } 
    return true;
}

/*删除第一个单元*/
bool PaceQueue::queueSent(PacketEvent *ev) {
    ev->sent = 1;
    if (totalSize > ev->size) {
	totalSize -= ev->size;
    } else {
	totalSize = 0;
    }
    queueFinalize();
    return true;
} 

int PaceQueue::queueOldest() const {
    if (oldestTs == -1)
	return BaseTimer::getCurrentTime();

    return oldestTs;
}

bool PaceQueue::queueEmpty() const {
    return cache->size() == 0 ? true : false;
}
/*
 * 500MS发送完毕所有报恩所需要的时间 
 * 队列里已有(nowTs - oldestTs)时间间隔的报文 
 * 500ms发送完毕还需要500 - space这些时间
 */
uint32_t PaceQueue::targetBitrateKbps(int64_t nowTs) {
    uint32_t ret = 0, space;
    if (oldestTs != -1 && nowTs > oldestTs) {
	space = (uint32_t)(nowTs - oldestTs);
        if (space >= MAX_SPACE_QUEUE_MS)
	    space = 1;
	else 
	    space = MAX_SPACE_QUEUE_MS - space;	    
    } else {
	space = MAX_SPACE_QUEUE_MS - 1;
    }

    if (cache->size() > 0 && totalSize > 0) {
	ret = totalSize * 8 / space;
    }
    return ret;
} 
