#include "ConnectMessage.h"
#include <iostream>

bool BoeConnectMessage::Process(const char *recvBuff, const BoeHeader *header) {
    const char *p = recvBuff + header->header_size;
    cid = *(uint32_t *)p;
    p += 4;
    tokenSize = *(uint16_t *)p;
    p += 2;
    for (int i = 0; i < TOKEN_SIZE; i++) {
        token[i] = (uint8_t)*p;
	i++;
    }
    ccType = *p;
    return true;
}

bool BoeConnectMessage::Build(char *buildMsg) const{
    char *p = buildMsg;
    *(uint32_t *)p = cid;
    p += 4;
    *(uint16_t *)p = tokenSize;
    p += 2;
    for (int i = 0; i < TOKEN_SIZE; i++) {
        *p = token[i];
	p++;
    }
    *(uint8_t *)p = ccType;
    return true;
}
