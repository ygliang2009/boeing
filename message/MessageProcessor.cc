#include "MessageProcessor.h"
#include <iostream>

bool BoeMsgProcessor::procConnectMsg(const char *recvBuff, const BoeHeader *header, char *respBuff) {
    BoeConnectMessage connectMsg;
    if (!connectMsg.Process(recvBuff, header)) {
	return false;
    }

    BoeHeader ackHeader;
    ackHeader.version = header->version;
    ackHeader.uid = header->uid;
    ackHeader.mid = MSG_CONNECT_ACK;
    if(!ackHeader.headerEncode(respBuff)) {
	return false;
    }
    char *ackBody = respBuff + header->header_size;
    BoeConnectAckMessage ackMsg;
    ackMsg.cid = connectMsg.cid;
    ackMsg.result = 0;

    if (!ackMsg.Build(ackBody))
        return false;

    return true;
}

bool BoeMsgProcessor::procSegmentMsg(Session *session, char *recvBuff, BoeHeader* header, char *ackBuff) {
    BoeSegmentMessage segmentMsg;
    if (!segmentMsg.Process(recvBuff, header)) {
	return false;
    }
    BoeReceiver *recv = session->getReceiver();

    if (!recv->putMessage(&segmentMsg)) {
	return false;
    }
    if (!recv->ackSegmentMessage(ackBuff, segmentMsg.packetId)) {
	return false;
    }
    return true;
}
