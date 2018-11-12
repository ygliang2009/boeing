#include "../catch/catch.hpp"
#include "../message/Header.h"
#include <cstdio>
#include "../message/MessageProcessor.h"

TEST_CASE( "BoeMsgProcessor.procConnectMsg" ) {
    char *recvBuff = (char *)malloc(sizeof(char) * 100);
    char *sendBuff = (char *)malloc(sizeof(char) * 100);
    memset(recvBuff, 0, 100);
    memset(sendBuff, 0, 100);
    BoeHeader header;
    header.mid = MSG_CONNECT;
    header.uid = 12;
    header.version = 1;
    header.header_size = 6;
    char *p = &recvBuff[6];
    //cid
    *(uint32_t *)p = 100;
    p += 4; 
    *(uint16_t *)p = 5;
    p += 2;
    *(uint8_t *)p = 2;  

    bool processResult = BoeMsgProcessor::procConnectMsg(recvBuff, &header, sendBuff);        	
    REQUIRE(processResult == true);

    uint64_t resTestHeader = 0xc1201;
    uint64_t resTestBody = 0x64; 
    char *q = sendBuff;
    //Header 48位
    uint64_t resHeader = *(uint64_t *)q & 0xFFFFFFFFFFFF;
    REQUIRE(resHeader == resTestHeader);
    q += 6;
    uint64_t resBody = *(uint64_t *)q;
    REQUIRE(resBody == resTestBody);
}
