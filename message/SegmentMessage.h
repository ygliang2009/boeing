#ifndef __SEGMENT_MESSAGE_H_
#define __SEGMENT_MESSAGE_H_

#include "Message.h"

class BoeSegmentMessage:public BoeMessage {
public:
    /*包序号*/
    uint32_t packetId;
    /*帧序号*/
    uint32_t fid;
    /*帧产生的相对时间间隔（相对于Sender创建时间）*/
    uint64_t interval;
    /*帧分包序号*/
    uint16_t index;
    /*帧分包总数*/
    uint16_t total;
    /*视频帧类型*/
    uint8_t ftype;
    /*编码器类型*/
    uint8_t payloadType;
    /*发送时刻相对帧产生时刻的相对时间间隔*/
    uint16_t sendInterval;
    /*传输通道序号，这个是传输通道每发送一个报文，它就增长1，而且重发报文也会增长*/
    uint16_t transportSeq;

    uint16_t dataSize;

    uint8_t data[VIDEO_SIZE];

public:
    BoeSegmentMessage() {};
    ~BoeSegmentMessage() {};

    bool Process(const char *, const BoeHeader *);
    bool Build(char *) const;
    bool isNull() const;
    BoeSegmentMessage& operator =(const BoeSegmentMessage &);
};

#endif
