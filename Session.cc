#include "Session.h"
#include "message/MessageType.h"

Session::Session() {
    __conn = new Connection();
    /*初始化的时候，如果不将sender或receiver设置为NULL，则sender receiver便成为野指针*/
    sender = NULL;
    recv = NULL;
    listener = NULL;

    interrupt = NET_STATE_NORMAL;
    state = SESSION_STATE_IDLE;

    uid = 0;
    rtt = 100;
    rttVar = 0; 
    scid = rand();
}

bool Session::resetSession() {
    
    CLEANUP(__conn);
    CLEANUP(sender);
    CLEANUP(recv);
    CLEANUP(listener);
    
    __conn = new Connection();

    interrupt = NET_STATE_NORMAL;

    uid = 0;
    rtt = 100;
    rttVar = 0; 
    scid = rand();

    DELETE_TIMESTATE_MEMBERS();
    initSession();
    return true;
}


Session::~Session() {
    CLEANUP(__conn);
    CLEANUP(sender);
    CLEANUP(recv);
    CLEANUP(listener);

    DELETE_TIMESTATE_MEMBERS(); 
}

bool Session::initSession() {
    if (!__conn->initConn())
        return false;
    
    INIT_TIMESTATE_MEMBER(\
	TIMESTATE_CONNECT_NO, TICK_DELAY_MS, &Session::sendConnectState);
    INIT_TIMESTATE_MEMBER(\
	TIMESTATE_DISCONNECT_NO, TICK_DELAY_MS, &Session::sendDisconnectState);
    INIT_TIMESTATE_MEMBER(\
	TIMESTATE_PING_NO, TICK_DELAY_MS/4, &Session::sendPingState);

    return true;
}

bool Session::registeListener(Listener *list) {
    if (listener != NULL)
        listener = list;
     return true;
}

ISender* Session::createSender() {
    if (sender == NULL)
        sender = new BoeSender();

    sender->setSession(this);
    sender->registeCaller(this);
    return sender;
}

bool Session::destroySender() {
    CLEANUP(sender);
    return true;
}

IReceiver* Session::createReceiver() {
    if (recv == NULL)
        recv = new BoeReceiver();

    recv->setSession(this);
    recv->createComponent();
    recv->firState = FIR_NORMAL;
    recv->setActive(uid);

    return recv;
}

bool Session::destroyReceiver() {
    CLEANUP(recv);
    return true;
}

bool Session::sessHeartbeat(const int64_t &nowTs) {
    if (recv == NULL)
        return false;
    recv->procRecvHeartbeat(nowTs);

    switch (state) {
        case SESSION_STATE_CONNECTING:
	    __sessOnTimer(nowTs, TIMESTATE_CONNECT_NO);
	    break;
 
	case SESSION_STATE_CONNECTED:
            if (sender != NULL)
		sender->procSenderHeartbeat(nowTs);
	    break;

	case SESSION_STATE_DISCONNECT:
	    __sessOnTimer(nowTs, TIMESTATE_DISCONNECT_NO);
	    break;

	default:
	    ; 
    }
    if (sender != NULL || recv != NULL) {
	__sessOnTimer(nowTs, TIMESTATE_PING_NO);
    }
    return true;
}

bool Session::__sessOnTimer(const int64_t &nowTs, uint8_t stateNo) {
    std::map<uint8_t, TimeStateMember *>::iterator it = timeStateTable.find(stateNo);
    if (it == timeStateTable.end() || it->second == NULL)
        return false;
    
    TimeStateMember *member = timeStateTable[stateNo];
    if (member->commandTs + member->tickMs < nowTs) {
	if (member->resend * member->tickMs < 10000) {
   	    (this->*(member->stateCallback))(nowTs);	    
        } else {
	    /*resend次数大于10次，证明超时了*/
            listener->notifyCallback(this, STOP_PLAY_NOTIFY);
	    resetSession();
        }
    } 
    return true;
}

/*供上层调用*/
bool Session::notify(uint32_t notifyNo) {
    if (notifyNo > 512)
        return false;

    if (listener == NULL)
	return false;

    listener->notifyCallback(this, notifyNo);
    return true;
}

bool Session::sendConnectState(const int64_t &nowTs) {
    /*update until online */
    char *sendingBuff = (char *)malloc(sizeof(char) * MTU); 
    BoeHeader header;
    header.uid = uid;
    header.mid = MSG_CONNECT;
    header.headerEncode(sendingBuff); 
    sendingBuff += header.header_size;
    BoeConnectMessage connectMsg;
    /*connect id 随机生成*/
    connectMsg.cid = scid;
    /*for test*/
    connectMsg.tokenSize = 0;
    /*congestion ctrl type*/
    connectMsg.ccType = 0;
    connectMsg.Build(sendingBuff);

    if (__conn == NULL)
        return false;

    __conn->sendPacket(sendingBuff);
    free(sendingBuff);
    
    UPDATE_TIMESTATE_COMMANDTS(TIMESTATE_CONNECT_NO);
    INC_TIMESTATE_RESEND(TIMESTATE_CONNECT_NO);
     
    return true;
}

bool Session::sendDisconnectState(const int64_t &nowTs) {
    /*update until online */
    char *sendingBuff = (char *)malloc(sizeof(char) * MTU); 
    BoeHeader header;
    header.uid = uid;
    header.mid = MSG_DISCONNECT;
    header.headerEncode(sendingBuff); 
    sendingBuff += header.header_size;

    BoeDisConnectMessage disConnectMsg;
    /*connect id 随机生成*/
    disConnectMsg.cid = scid;
    disConnectMsg.Build(sendingBuff);
    if (__conn == NULL)
        return false;

    __conn->sendPacket(sendingBuff);
    free(sendingBuff);
    
    UPDATE_TIMESTATE_COMMANDTS(TIMESTATE_DISCONNECT_NO);
    INC_TIMESTATE_RESEND(TIMESTATE_DISCONNECT_NO);
    return true;
}

bool Session::sendPingState(const int64_t &nowTs) {
    BoeHeader header;
    header.uid = uid;
    header.mid = MSG_PING;

    char *sendingBuff = (char *)malloc(sizeof(char) * header.header_size);
    header.headerEncode(sendingBuff);

    if (__conn == NULL) 
        return false;
    
    __conn->sendPacket(sendingBuff);
    free(sendingBuff);

    /*更新state状态*/
    timeStateTable[TIMESTATE_PING_NO]->commandTs = nowTs;
    timeStateTable[TIMESTATE_PING_NO]->resend++;
    
    /*网络超时3s钟，不进行报文转发 */
    if (timeStateTable[TIMESTATE_PING_NO]->resend > 12) {
	interrupt = NET_STATE_INTERRUPT;
        if (listener != NULL)
            listener->notifyCallback(this, NET_INTERRUPT_NOTIFY);
	else {
	    /*日志打Warning*/
        }
    }
    return true;
}

/*完成一次PING PONG会调用这个函数*/
bool Session::sessCalculateRtt(const uint32_t &keepRtt) {
    /*参考TCP RTT计算方法：Jacobson/Karels 算法 */
    uint32_t keepRttVar = keepRtt;
    if (keepRttVar < 5)
        keepRttVar = 5;
    /*
     * 当前的这个rtt仅占比25%
     * rttVar表示网络抖动之间的差值
     */
    rttVar = (rttVar * 3 + BOE_ABS(rtt, keepRttVar))/4;
    if (rttVar < 10)
        rttVar = 10;
    rtt = (7 * rtt + keepRttVar)/8;
    if (rtt < 10)
        rtt = 10;
    /*
     * 当前RTT = SRTT + RTTVar (SRTT代表平滑的RTT，RTTVar 代表网络抖动之间的差值)
     */
    if (sender != NULL)
        sender->updateRtt(rtt + rttVar);
    if (recv != NULL)
        recv->updateRtt(rtt + rttVar);

    return true;
}

/*真正发送SegmentMessage消息*/
bool Session::sendSegmentMessage(const BoeSegmentMessage *segmentMessage) {
    char *sendingBuffer = (char *)malloc(sizeof(char) * MTU);
    char *p = sendingBuffer;

    BoeHeader header;
    header.uid = uid;
    header.mid = MSG_SEGMENT;
    header.headerEncode(p);

    p += header.header_size; 
    segmentMessage->Build(p);
    __conn->sendPacket(p) ;

    free(p);      
    return true;
}

/*
 * 从Sender层触发的回调，当网络发生变化时，最后会执行到这个函数
 */
bool Session::onTargetTransferRate(TargetTransferRate transferRate) {
    /*通知receiver，降低feedback调用次数*/
    if (recv == NULL)
         return false;
      
    recv->onBitrateChange(transferRate.targetRate);
    return true;
} 
