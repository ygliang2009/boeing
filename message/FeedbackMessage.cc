#include "FeedbackMessage.h"

BoeFeedbackPacket::BoeFeedbackPacket() {
    createTs = -1;
    arrivalTs = -1;
    sendTs = -1;
    sequenceNumber = 0;
    payloadSize = 0;
}

BoeFeedbackPacket::~BoeFeedbackPacket() {

}

bool BoeFeedbackMessage::Process(const char *recvBuff, const BoeHeader *header) {
    const char *p = recvBuff + header->header_size;
    flag = *(uint8_t *)p;
    p++;
    fractionLoss = *(uint8_t *)p;
    p++;
    packetNum = *(int32_t *)p;
    p += sizeof(int32_t);
    baseSeq = *(int64_t *)p;
    p += 8;
    minTs = *(int64_t *)p;
    p += 8;
    samplesNum = *(uint8_t *)p;
    p++;
    for (int i = 0; i < MAX_FEEDBACK_COUNT; i++) {
        samples[i].seq = *(uint16_t *)p;
	p += 2;
        samples[i].ts = *(int64_t *)p;
 	p += 8;	
    }
    return true;
}


bool BoeFeedbackMessage::Build(char *recvBuff) const {
    char *p = recvBuff;
    *(uint8_t *)p = flag;
    p++;
    *(uint8_t *)p = fractionLoss;
    p++;
    *(int32_t *)p = packetNum;
    p += sizeof(int);
    *(int64_t *)p = baseSeq;
    p += 8;
    *(int64_t *)p = minTs;
    p += 8;
    *(uint8_t *)p = samplesNum;
    p++;

    for (int i = 0; i < MAX_FEEDBACK_COUNT; i++) {
        *(uint16_t *)p = samples[i].seq ;
	p += 2;
        *(int64_t *)p = samples[i].ts;
 	p += 8;	
    }
    return true;
}
