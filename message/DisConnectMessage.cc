#include "DisConnectMessage.h"

bool BoeDisConnectMessage::Process(const char *recvBuff, const BoeHeader *header) {
    const char *p = recvBuff + header->header_size;
    cid = *(uint32_t *)p;
    return true;
};

bool BoeDisConnectMessage::Build(char *buildMsg) const {
    char *p = buildMsg;
    *(uint32_t *)p = cid;
    return true;
};
