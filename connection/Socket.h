#ifndef __SOCKET_H_
#define __SOCKET_H_
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define MTU_NUM	1024

using namespace std;

class Socket{
private:
    int64_t sockFd;
    string sockAddr;
    int64_t sockPort;
    int32_t addrLen;
    struct sockaddr_in addrServ;

public:
    Socket();
    ~Socket();
    void Init(string &, int64_t);
    bool Create();
    size_t Send(char *) const;
    size_t Recv(char *) const;
    void Close();
};


#endif
