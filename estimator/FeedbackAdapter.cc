#include "FeedbackAdapter.h"

FeedbackAdapter::FeedbackAdapter() {
    int i = 0;
    for (; i < FEEDBACK_RTT_WIN_SIZE; ++i) {
	rtts[i] = -1;
    }
    minFeedbackRtt = 10;
    num = 0;
    index = 0;
    history = new SenderHistory();
}

FeedbackAdapter::~FeedbackAdapter() {
    CLEANUP(history);
}

bool FeedbackAdapter::addPacket(const uint64_t transSeq, const size_t transSize) {
    if (history == NULL)
        return false;

    FeedbackPacket *packet = new FeedbackPacket();
    packet->arrivalTs = -1;
    packet->createTs = packet->sendTs = BaseTimer::getCurrentTime();
    packet->payloadSize = transSize;
    packet->sequenceNumber = transSeq;
    history->historyAdd(packet);    
    return true;
}

bool FeedbackAdapter::adapterOnFeedback(const BoeFeedbackMessage *feedbackMessage) {
   /*
    * 将Feedback消息中的Feedback Sample信息更新到packets存储中
    * 并更新packets存储中的FeedbackPacket其他未填充信息
    * 计算RTT值更新到rtts存储中
    */ 
    int32_t feedbackRtt = 0;
    int64_t nowTs = BaseTimer::getCurrentTime();
    for (uint32_t i = 0; i < feedbackMessage->samplesNum; i++) {
	/*从history结构中查找出FeedBackPacket结构，并删除history中的数据记录*/
        if (history->historyGet(feedbackMessage->samples[i].seq, &(packets[num]), 1)) {
	    if (packets[num].sendTs > 0) {
		feedbackRtt = BOE_MAX(nowTs - packets[num].sendTs, feedbackRtt);
	        rtts[index++ % FEEDBACK_RTT_WIN_SIZE] = feedbackRtt;
	    }
	    packets[num].arrivalTs = feedbackMessage->samples[i].ts;
	    num++;
        }
    }
    /*更新报文与反馈的RTT最小值*/  
    if (feedbackRtt > 0)  {
	minFeedbackRtt = rtts[0];
        for (int i = 1; i < FEEDBACK_RTT_WIN_SIZE; i++) {
	    if (minFeedbackRtt > rtts[i] && rtts[i] > 0) 
		minFeedbackRtt = rtts[i];
        }
    }

    __feedbackQsort();
    return true;
}

int feedbackCompare(const void *arg1, const void *arg2) {
    FeedbackPacket *p1, *p2;
    p1 = (FeedbackPacket *)arg1;
    p2 = (FeedbackPacket *)arg2;
    if (p1->arrivalTs < p2->arrivalTs)
        return -1;
    else if (p1->arrivalTs > p2->arrivalTs)
	return 1;
    else if (p1->sendTs < p2->sendTs)
	return -1;
    else
	return 1;
}

bool FeedbackAdapter::__feedbackQsort() {
    qsort(rtts, FEEDBACK_RTT_WIN_SIZE, sizeof(FeedbackPacket), feedbackCompare);
    return true;
}
