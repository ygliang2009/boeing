#include "PingMessage.h"

bool BoePingMessage::Build(char *buildMsg) const {
    char *p = buildMsg;
    *(uint32_t *)p = ts;
    return true;
}

bool BoePingMessage::Process(const char* recvBuff, const BoeHeader *header) {
    const char *p = recvBuff + header->header_size;
    ts = *(uint32_t *)p;
    return true; 
}
