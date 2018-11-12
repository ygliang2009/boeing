#include "Connection.h"

Connection::Connection() {
    recvBuff = NULL; 
    mtu = 1000;  
    addr = "127.0.0.1";
    port = 9955; 
}

Connection::~Connection() {
    recvBuff = NULL;
    mtu = 0;
    addr = "";
    port = -1;
}

bool Connection::initConn() {
    recvBuff = (char *)malloc(sizeof(char) * mtu);
    sock.Init(addr, port);
    return sock.Create();
}

size_t Connection::recvPacket() {
    std::cout << "recvPacket" << std::endl;
    size_t recvLen = sock.Recv(recvBuff);
    return recvLen;
}

size_t Connection::sendPacket(char *sendBuf) {
    return sock.Send(sendBuf);
}

void Connection::dumpBuffer() {
    if (strlen(recvBuff) > 0) {
        std::cout << recvBuff << std::endl;
    }
}
