#ifndef __FEEDBACK_PACKET_H_
#define __FEEDBACK_PACKET_H_
/*很多文件都需要引入FeedbackPacket头文件，故将这个结构独立出来*/
class FeedbackPacket {
public:
    /*创建时间戳*/
    int64_t createTs;
    /*到达时间戳*/
    int64_t arrivalTs;
    /*发送时间戳*/
    int64_t sendTs;
    /*发送通道的报文序号*/
    uint16_t sequenceNumber;
    /*包数据大小*/
    size_t payloadSize;     
};
#endif
