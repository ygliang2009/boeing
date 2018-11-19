#ifndef __SESSION_H_
#define __SESSION_H_

#include "Connection.h"
#include "transport/Sender.h"
#include "transport/Receiver.h"
#include "interface/Listener.h"
#include "interface/NotifyType.h"
#include "interface/Caller.h"

#define TICK_DELAY_MS		1000

#define INIT_TIMESTATE_MEMBER(stateNo_, tickMs_, stateCb_)         \
    do { 					     		   \
        TimeStateMember *member = new TimeStateMember();     	   \
        member->stateNo = (stateNo_);                \
        member->tickMs = (tickMs_);                  \
        member->stateCallback = (stateCb_);          \
        member->resend = 0;                          \
        timeStateTable[(stateNo_)] = member;             \
     } while (0)

#define DELETE_TIMESTATE_MEMBERS()  \
    std::map<uint8_t, TimeStateMember*>::iterator iter = timeStateTable.begin(); \
    for (; iter != timeStateTable.end(); iter++) { \
        if (iter->second != NULL) { \
	    delete iter->second; \
	    iter->second = NULL; \
        } \
    } \
    timeStateTable.clear()

#define INC_TIMESTATE_RESEND(stateNo) \
    do { \
        std::map<uint8_t, TimeStateMember*>::iterator it = timeStateTable.find((stateNo)); \
        if (it != timeStateTable.end()) { \
            (it->second)->resend++; \
        } \
    }while(0)

#define UPDATE_TIMESTATE_COMMANDTS(stateNo) \
    std::map<uint8_t, TimeStateMember*>::iterator it = timeStateTable.find((stateNo)); \
    if (it != timeStateTable.end()) { \
         it->second->commandTs = BaseTimer::getCurrentTime();\
    }


#define CLEAR_TIMESTATE_RESEND(stateNo) \
    do { \
        std::map<uint8_t, TimeStateMember*>::iterator cit = timeStateTable.find((stateNo)); \
        if (cit != timeStateTable.end()) { \
            (cit->second)->resend = 0; \
        } \
    }while (0) 
    

enum {
    TIMESTATE_CONNECT_NO = 1,
    TIMESTATE_DISCONNECT_NO,
    TIMESTATE_PING_NO,
};

enum {
    SESSION_STATE_IDLE = 0x00,
    
    SESSION_STATE_CONNECTING,
    SESSION_STATE_CONNECTED,
    SESSION_STATE_DISCONNECT,
};

enum {
    /*正常的网络连通情况*/
    NET_STATE_NORMAL = 0x01,
    /*网络异常情况，连续ping 12次超时，则置为interrupt状态*/
    NET_STATE_INTERRUPT,
    /*网络恢复*/
    NET_STATE_RECOVER
};

class BoeSender;
class BoeReceiver;

typedef bool (Session::*StateCallback)(const int64_t &nowTs);

class TimeStateMember {
public:
    uint8_t stateNo;
    int64_t tickMs;
    uint32_t resend;
    int64_t commandTs;
    StateCallback stateCallback;
};


class Session : public Caller{
private:
    Connection *__conn;

public:
    
    Listener *listener;

    uint8_t state;
    uint8_t interrupt;   
    uint64_t rtt;
    uint64_t rttVar;
    
    uint32_t uid; 
   
    /*发送方的connect id, 32位随机数*/ 
    uint32_t scid;
    /*接收方的connect id*/ 
    uint32_t rcid;
    
    std::map<uint8_t, TimeStateMember *> timeStateTable;

    BoeSender* getSender() const {
	return sender;
    }
    
    bool setSender(BoeSender *s) {
	if (sender != NULL)
	    sender = s;
	return true;
    }

    BoeReceiver* getReceiver() const {
	return recv;
    } 

    bool setReceiver(BoeReceiver *receiver) {
	if (recv != NULL)
	    recv = receiver;
	return true;
    }
public:
    Session();
    virtual ~Session();
  
    bool initSession();
    bool resetSession();
  
    void setConn(Connection* conn){
	__conn = conn;
    }
    Connection* getConn() {
        return __conn;
    }

    ISender* createSender();
    bool destroySender();

    IReceiver* createReceiver();
    bool destroyReceiver();

    bool sessHeartbeat(const int64_t &);
    bool sessCalculateRtt(const uint32_t &);
    
    bool registeListener(Listener *);

    bool sendSegmentMessage(const BoeSegmentMessage *);

    bool onTargetTransferRate(TargetTransferRate);    

public:
    /*timer里调用的state方法*/
    bool sendConnectState(const int64_t &);
    bool sendDisconnectState(const int64_t &);
    bool sendPingState(const int64_t &);
    bool notify(uint32_t);

private:
    bool __sessOnTimer(const int64_t &, uint8_t);

private:
    BoeSender *sender;  
    BoeReceiver *recv; 
};

#endif
