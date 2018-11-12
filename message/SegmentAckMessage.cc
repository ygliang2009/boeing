#include "SegmentAckMessage.h"

bool BoeSegmentAckMessage::Process(const char *recvBuff, const BoeHeader *header) {
    /*构造的长度不能超过1个MTU大小*/
    const char *p = recvBuff + header->header_size;
    basePacketId = *(uint32_t *)p;
    p += 4;
    ackedPacketId = *(uint32_t *)p;
    p += 4;
    nackNum = *(uint8_t *)p;
    p += 1;
    for (int i = 0; i < NACK_NUM; i++) {
	nack[i] = *(uint16_t *)p;
        p += 2;
    }
    return true;
};

bool BoeSegmentAckMessage::Build(char *buildMsg) const{
    char *p = buildMsg;
    *(uint32_t *)p = basePacketId;
    p += 4;
    *(uint32_t *)p = ackedPacketId;
    p += 4;
    *(uint8_t *)p = nackNum;
    p += 1;

    for (int i = 0; i < NACK_NUM; i++) {
	*(uint16_t *)p = nack[i];
	p += 2;
    }
    return true;
};
