#include "SegmentMessage.h"

bool BoeSegmentMessage::Process(const char *recvBuff, const BoeHeader *header) {
    const char *p = recvBuff + header->header_size;
    packetId = *(uint32_t *)p;
    p += 4;
    fid = *(uint32_t *)p;
    p += 4;
    interval = *(uint64_t *)p;
    p += 8;
    index = *(uint16_t *)p;
    p += 2;
    total = *(uint16_t *)p;
    p += 2;
    ftype = *(uint8_t *)p;
    p += 1;
    payloadType = *(uint8_t *)p;
    p += 1;
    sendInterval = *(uint16_t *)p;
    p += 2; 
    transportSeq = *(uint16_t *)p;
    p += 2;
    dataSize = *(uint16_t *)p;
    p += 2;
    for (int i = 0; i < VIDEO_SIZE; i++) {
	data[i] = *p;
    } 
    return true;
}

bool BoeSegmentMessage::Build(char *buildMsg) const{
    char *p = buildMsg;
    *(uint32_t *)p = packetId;
    p += 4;
    *(uint32_t *)p = fid;
    p += 4;
    *(uint64_t *)p = interval;
    p += 8;
    *(uint16_t *)p = index;
    p += 2;
    *(uint16_t *)p = total;
    p += 2;
    *(uint8_t *)p = ftype;
    p += 1;
    *(uint8_t *)p = payloadType;
    p += 1;
    *(uint16_t *)p = sendInterval;
    p += 2;
    *(uint16_t *)p = transportSeq;
    p += 2;
    *(uint16_t *)p = dataSize;
    p += 2;
    for (int i = 0; i < VIDEO_SIZE; i++) {
	*p = data[i];
    }
    return true;
}

bool BoeSegmentMessage::isNull() const {
    if (packetId == 0 && fid == 0 && total == 0)
        return true;
    return false;
}

BoeSegmentMessage& BoeSegmentMessage::operator =(const BoeSegmentMessage &target) {
    packetId = target.packetId;
    fid = target.fid;
    interval = target.interval;
    index = target.index;
    total = target.total;
    ftype = target.ftype;
    payloadType = target.payloadType;
    sendInterval = target.sendInterval;
    transportSeq = target.transportSeq;
    dataSize = target.dataSize;

    for (int i = 0; i < VIDEO_SIZE; i++) {
        data[i] = target.data[i];
    } 
    return *this;
}
