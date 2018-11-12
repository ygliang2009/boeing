#include "Loop.h"
#include "util/BaseTimer.h"
#include <iostream>
#include "transport/Sender.h"
#include "transport/Receiver.h"

bool LoopEvent::init() {
    loop_interval = 5;
    bool initRes = session.initSession();
    return initRes;
}

bool LoopEvent::run() {
    int64_t currentTs = BaseTimer::getCurrentTime();
    int64_t lastIntervalTs = currentTs;
    Connection *conn = session.getConn();
    
    while (true) {
        int64_t recvLen = conn->recvPacket();
	
        if (recvLen > 0) {
	     char *recvBuff = conn->getBuffer();
 	     BoeHeader header;
	     if (!header.headerDecode(recvBuff)) {
		 perror("header process error");
	         continue;
             }
	     __processMessage(recvBuff, &header);
	     
             CLEANUP(recvBuff);
        }
	
        currentTs = BaseTimer::getCurrentTime();
	if (currentTs - lastIntervalTs >= 5000 * 1000) {
	     //5ms执行一次
	    __processHeartbeat(currentTs);
	    lastIntervalTs = currentTs;
        }	
    }
    return true;
}

bool LoopEvent::__processHeartbeat(int64_t current_ts) {
    session.sessHeartbeat(current_ts);
    return true;
}

bool LoopEvent::__processMessage(char *recvBuff, BoeHeader *header) {
    if (header->mid > MAX_MSG_ID || header->mid < MIN_MSG_ID) {
        perror("error message id");
        return false;
    }
    if (session.state == NET_STATE_INTERRUPT) {
        /*只有网络连通性正常的条件下，程序才能走到这里*/
	session.state = NET_STATE_RECOVER;
    }

    Connection *conn = session.getConn();
    uint32_t uid = header->uid;

    switch (header->mid) {
        case MSG_CONNECT: {
	    char *ackBuff = (char *)malloc(sizeof(char) * 100);
	    memset(ackBuff, 0, 100);

	    if (BoeMsgProcessor::procConnectMsg(recvBuff, header, ackBuff)) {
                session.uid = uid;
                IReceiver* recv = session.createReceiver();
                if (recv == NULL)
                    break;
                conn->sendPacket(ackBuff);
	    }
	    free(ackBuff);
	    break;
        }
        case MSG_CONNECT_ACK: {
	    BoeConnectAckMessage connectAckMsg;
            connectAckMsg.Process(recvBuff, header);
 	    if (session.state != SESSION_STATE_CONNECTING) {
		return false;
            }
            if (connectAckMsg.result != 0) {
	        return false;
            }
            if (session.getSender() == NULL) {
	        ISender *sender = session.createSender();
                if (sender == NULL)
                    break;

                session.state = SESSION_STATE_CONNECTED;
	        session.interrupt = NET_STATE_RECOVER;
	    }
	    break;
	}
	case MSG_DISCONNECT: {
	    BoeDisConnectMessage disconnectMsg;
            disconnectMsg.Process(recvBuff, header);
            /*构造Disconnect ACK*/
	    char *disconnectAckBuff = (char *)malloc(sizeof(char) * MTU);
            char *p = disconnectAckBuff;
            BoeHeader respHeader;
	    respHeader.uid = session.uid;
	    respHeader.mid = MSG_DISCONNECT;
            respHeader.headerEncode(p);
    	    
            BoeDisConnectAckMessage disconnectAckMsg;
	    disconnectAckMsg.cid = disconnectMsg.cid;
            disconnectAckMsg.result = 0;

	    disconnectAckMsg.Build(p + respHeader.header_size);
	    conn->sendPacket(disconnectAckBuff);
	    free(disconnectAckBuff);

	    /*销毁receiver信息*/
            if (session.rcid == disconnectMsg.cid && session.getReceiver() != NULL) {
		/* 通知上层停止播放 */
	        session.notify(STOP_PLAY_NOTIFY);
	        session.destroyReceiver();
	        session.setReceiver(NULL);
            }
	    break;
	}
	case MSG_DISCONNECT_ACK: {
	    BoeDisConnectAckMessage disconnectAckMsg;
            disconnectAckMsg.Process(recvBuff, header);
	    
            if (session.state == SESSION_STATE_DISCONNECT) {
		session.resetSession();
	        session.notify(DISCONNECT_NOTIFY);
            }
	    break;
	}
        case MSG_SEGMENT: {
            if (session.getReceiver() == NULL)
	        return false;

	    char *ackBuff = (char *)malloc(sizeof(char) * MTU);
	    memset(ackBuff, 0, MTU);
            BoeMsgProcessor::procSegmentMsg(&session, recvBuff, header, ackBuff);
	    break;
	}
	case MSG_SEGMENT_ACK: {
	    BoeSegmentAckMessage segmentAckMsg;
            segmentAckMsg.Process(recvBuff, header);
	    /*处理Segment ACK消息*/
	    BoeSender *sender = session.getSender();

            if (sender != NULL) {
	        sender->procSegmentAck(&segmentAckMsg);
            }
	    break;
	}
	case MSG_FEEDBACK: {
	    BoeFeedbackMessage feedbackMsg;
            feedbackMsg.Process(recvBuff, header);
	    
            BoeSender *sender = session.getSender();
	    
            if (sender != NULL) {
	        sender->procFeedbackMsg(&feedbackMsg);
            }
	    break;
	}
	case MSG_PING: {
	    char *ackBuff = (char *)malloc(sizeof(char) * 100);
	    memset(ackBuff, 0, 100);

	    BoePingMessage pingMsg;
	    pingMsg.Process(recvBuff, header);
            BoeHeader header;
            header.uid = session.uid;
	    header.mid = MSG_PONG;
            header.headerEncode(ackBuff);
	    ackBuff += header.header_size;

            BoePongMessage pongMsg;
	    pongMsg.ts = pingMsg.ts;
            pongMsg.Build(ackBuff);

	    conn->sendPacket(ackBuff);
	    free(ackBuff);
        }
	case MSG_PONG: {
	    BoePongMessage pongMsg;
	    pongMsg.Process(recvBuff, header);

            int64_t nowTs = BaseTimer::getCurrentTime();
            uint32_t keepRtt = 5;

	    if (nowTs - pongMsg.ts > 5)
	        keepRtt = (uint32_t)(nowTs - pongMsg.ts);

	    session.sessCalculateRtt(keepRtt);
        }
	default:
	    break; 
    } 
    return true;
}
