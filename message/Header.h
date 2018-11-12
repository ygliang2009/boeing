#ifndef __BOE_HEADER_H_
#define __BOE_HEADER_H_
#include <stdint.h>

#define HEADER_VERSION		1

enum MessageId{
    MIN_MSG_ID = 0x10,
    
    MSG_CONNECT,
    MSG_CONNECT_ACK,
    
    MSG_DISCONNECT,
    MSG_DISCONNECT_ACK,

    MSG_PING,
    MSG_PONG,
    
    MSG_SEGMENT,
    MSG_SEGMENT_ACK,
    
    MSG_FEEDBACK,
    
    MAX_MSG_ID
};


class BoeHeader {
public:
    int8_t version;
    int8_t mid;
    uint32_t uid;
    int32_t header_size;

public:
    BoeHeader();
    ~BoeHeader();
    bool headerEncode(char *) const;
    bool headerDecode(char *);
};
#endif
