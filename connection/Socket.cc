#include "Socket.h"
#include <iostream>

using namespace std;

Socket::Socket() {

}

Socket::~Socket() {
    sockAddr = "";
    sockPort = -1;
    sockFd = -1;
    close(sockFd);
}


void Socket::Close() {
    sockAddr = "";
    sockPort = -1;
    sockFd = -1;
    close(sockFd);
}

void Socket::Init(string &sockaddr, int64_t sockport) {
    sockAddr = sockaddr;
    sockPort = sockport;
} 

bool Socket::Create() {
    sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd < 0) {
	return false;
    } 
   
    int flags = 0;
    if (fcntl(sockFd, F_GETFL, 0) < 0 || fcntl(sockFd, F_SETFL, flags|O_NONBLOCK) < 0) {
	close(sockFd);
        return false;
    }  
    
    int bufSize = 1024 * 1024;
    setsockopt(sockFd, SOL_SOCKET, SO_SNDBUF, &bufSize, sizeof(int));
    setsockopt(sockFd, SOL_SOCKET, SO_RCVBUF, &bufSize, sizeof(int));
    
    memset(&addrServ, 0, sizeof(addrServ));

    addrServ.sin_family = AF_INET;
    addrServ.sin_addr.s_addr = inet_addr(sockAddr.c_str());
    addrServ.sin_port = htons(sockPort);
    addrLen = sizeof(addrServ);

    if (bind(sockFd, (struct sockaddr *)&addrServ, addrLen) == -1) {
        close(sockFd);
        return false;
    }
    return true;
}

size_t Socket::Send(char* sendBuf) const{
    return sendto(sockFd, sendBuf, sizeof(sendBuf), 0, (struct sockaddr *)&addrServ, addrLen);
}

size_t Socket::Recv(char* recvBuf) const{
    fd_set fds;
    struct timeval timeout = {0, 5000 * 1000};   
    FD_ZERO(&fds);
    FD_SET(sockFd, &fds);
    switch (select(sockFd + 1, &fds, NULL, NULL, &timeout)) {
        case -1:
 		return -1;
	case 0:
		return 0;
	default:
		break;
    }
    if (!(FD_ISSET(sockFd, &fds)))
	return -1;
    return recvfrom(sockFd, recvBuf, sizeof(recvBuf), 0, (struct sockaddr *)&addrServ, (socklen_t *)&addrLen);
}
