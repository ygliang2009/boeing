#ifndef __CONNECTION_H_
#define __CONNECTION_H_

#include <stdlib.h>
#include <sys/socket.h>
#include "Socket.h"
#include <iostream>
#include <cstring>

using namespace std;

class Connection {
private:
    Socket sock;
    uint16_t mtu;
    char* recvBuff;
    string addr;
    int64_t port;

public:
    Connection();
    virtual ~Connection();

    bool initConn();
    size_t recvPacket();
    size_t sendPacket(char *);    

    char* getBuffer() {
	return recvBuff;
    }

    void dumpBuffer();
};

#endif
