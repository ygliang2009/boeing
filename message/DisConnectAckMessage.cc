#include "DisConnectAckMessage.h"

bool BoeDisConnectAckMessage::Process(const char *recvBuff, const BoeHeader *header) {
    const char *p = recvBuff + header->header_size;
    cid = *(uint32_t *)p;
    p += 4;
    result = *(uint32_t *)p;
    return true;
};

bool BoeDisConnectAckMessage::Build(char *buildMsg) const{
    char *p = buildMsg;
    *(uint32_t *)p = cid;
    p += 4;
    *(uint32_t *)p = result;
    return true;
};
