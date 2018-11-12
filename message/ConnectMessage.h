#ifndef __CONNECT_MESSAGE_H_
#define __CONNECT_MESSAGE_H_

#include "Message.h"

class BoeConnectMessage:public BoeMessage {
public:
    /*Connect id 协商的呼叫号，每次一个随机数，和disconnect等消息保持一致*/
    uint32_t cid;			
    /*token是一个验证信息，可以用类似证书的方式来进行验证 */
    uint16_t tokenSize;		
    uint8_t token[TOKEN_SIZE]; 
    uint8_t ccType;

public:
    BoeConnectMessage() {};
    ~BoeConnectMessage() {};
    bool Process(const char *, const BoeHeader *);
    bool Build(char *) const;
};

#endif
