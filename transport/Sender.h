#ifndef __SENDER_H_
#define __SENDER_H_
#include "Session.h"
#include "SenderLimiter.h"
#include "message/SegmentMessage.h"
#include "message/SegmentAckMessage.h"
#include <map>
#include "SendCongestionCtrl.h"
#include "Trigger.h"
#include "Caller.h"

#define SPLIT_NUMBER		1024

class Session;
class BoeSendCongestionCtrl;

/*
 *  Sender一共持有三个主要的缓存结构：
 *    sendingCache，history cache 和 pacing. 
 *    sendingCache是缓存数据包所用，NACK重发也是在这个数据结构中实现
 *    history cache保存数据的发送信息，主要为了验证feedback返回的sample信息所用
 *    pacing结构主要做消息的发送控制
 */

/*
 *  一个帧的时间是这样计算的
 *    
 *    接收时间 = 填充到缓存的相对时间(segmentMessage.interval) + \
 *         开始发送耗费的时间(segmentMessage.sendInterval) +  RTT +  基准时间(sender.firstTs) 
 *
 */
class BoeSender : public Trigger, public ISender{
public:
    Session* session;
    /* 发送窗口 */
    std::map<uint64_t, BoeSegmentMessage*> sendingCache;
    
    /* 发送端拥塞控制模块 */
    BoeSendCongestionCtrl *scc;   
    /* 发送速率控制 */
    SenderLimiter *limiter; 
    /*上一层的回调函数*/
    Caller *caller;    
    int actived;
    /*接收端报告的已收到的最大连续报文*/
    uint32_t basePacketId; 
    /*发送的packet编号，自增*/
    uint32_t packetIdSeed;
    /*发送的frame编号，自增*/
    uint32_t frameIdSeed;
    /*第一帧起始时间戳*/
    int64_t firstTs;
    /*生成发送侧唯一ID值，标识每次发送的SegmentMessage的transportSeq ID值*/
    int64_t transportSeqSeed;

public:
    BoeSender();

    ~BoeSender();
    
    bool heartBeat();
   
    bool addPacket();
    
    bool setSession(Session *sess) {
	session = sess;
        return true;
    }

    bool createSendingCache();

    bool procSenderHeartbeat(const int32_t &currTs);

    bool networkChangeTrigger(uint32_t, uint8_t, uint32_t);

    bool updateRtt(const uint32_t &) const;

    bool addPackets(const uint8_t &, const uint8_t &, const uint8_t*, const size_t &);

    bool procSegmentAck(BoeSegmentAckMessage* );

    bool sendCallback(const uint32_t &, const int &, const size_t &);
    /*处理feedback消息*/
    bool procFeedbackMsg(BoeFeedbackMessage*);
    /*注册call函数*/
    bool registeCaller(Caller *);

private:
    uint16_t __splitFrame(uint16_t[], const size_t &);
    bool __upCacheAccodingBase(const uint32_t &);
};

#endif
