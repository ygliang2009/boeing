#ifndef __RECEIVER_H_
#define __RECEIVER_H_
#include <map>
#include <stdint.h>
#include "message/SegmentMessage.h"
#include "message/SegmentAckMessage.h"
#include "RecvCongestionCtrl.h"
#include "Session.h"
#include "Internal.h"
#include "IReceiver.h"

#define CACHE_SIZE	1024
#define SEGMENT_HEADER_SIZE	10
#define INDEX(x) 	x%CACHE_SIZE

class BoeRecvCongestionCtrl;
class Session;

enum {
    BUFFER_WAITING = 0,
    BUFFER_PLAYING
};

enum {
    FIR_NORMAL = 0,
    FIR_FLIGHTTING
};

typedef  uint32_t	LossKey;
//class LossKey {
//public:
//    uint64_t seq;
//};

class LossValue {
public:
    uint32_t lossCount;
    uint64_t ts;
};


class BoeFrame {
public:
    //frame id
    uint32_t fid; 
    
    uint32_t lastSeq;
    uint32_t ts;
    
    int frameType;

    int segNumber;
    
    BoeSegmentMessage *segments; 

public:
    BoeFrame();
    ~BoeFrame(); 
};

class FrameCache {
public:
    uint32_t size;
    
    uint32_t minSeq;
    
    uint32_t minFid;
    uint32_t maxFid;
    /*缓冲区播放帧的时间*/
    uint32_t playFrameTs;
    /*缓冲区中最大的ts*/
    uint32_t maxTs;
    /*已经播放的相对视频时间戳*/
    uint32_t frameTs;
    /*当前系统时间的时间戳*/
    uint64_t playTs;
    /*帧间隔时间*/
    uint32_t frameTimer;
    /*
     * cache缓冲时间，以毫秒为单位
     * 这个cache时间长度是 > rtt + 2 * rtt_val
     * 根据重发报文的次数来决定 
     */
    uint32_t waitTimer;
    
    int state;
    int lossFlag;
    float f;

    BoeFrame *frames;

public:
    FrameCache();
    ~FrameCache();   
};


class BoeReceiver : public IReceiver{
public:
    BoeReceiver();
    ~BoeReceiver();

    void setBaseUid(const uint64_t &);
    bool heartBeat();
    /*接收端拥塞控制对象心跳，建议每5ms一次*/ 
    bool onReceived();    
    bool putMessage(const BoeSegmentMessage *);
    /*设置配置的最大码率*/
    bool setMaxBitrate();
    /*设置配置的最小码率*/
    bool setMinBitrate(); 
  
    void setSession(Session *sess) {session = sess;}

public:
    //丢失报文队列
    std::map<LossKey, LossValue> lossMap;
    //接收完毕报文队列
    uint32_t baseSeq;
    uint32_t baseUid;
    uint32_t maxSeq;
    uint32_t maxTs;

    int lossCount;
    //确认发出的时间戳
    uint64_t ackTs;
    //调节播放缓存wait_timer的时间
    uint64_t cacheTs;
    uint64_t activeTs;

    int actived;

    /*和FIR有关的参数*/
    uint32_t firSeq;
    /*请求关键帧的消息seq*/
    int firState;

    FrameCache *cache;
    BoeRecvCongestionCtrl *rcc;
    Session *session;

public:
    bool setActive(const uint32_t &);
    bool ackSegmentMessage(char *, const uint32_t &); 
    bool procRecvHeartbeat(const uint64_t &);
    bool createComponent();
    bool updateRtt(const uint32_t &);

    bool onBitrateChange(const uint32_t &);

private:
    bool __createFrameCache();
    bool __createCongestionCtrl();
    bool __updateLossMap(const uint32_t &, const uint32_t &);
    bool __updateFrameCache(const BoeSegmentMessage *);
    bool __updateCacheWaitTimer(const uint32_t &);
};

#endif
