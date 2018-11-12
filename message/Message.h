#ifndef __BOE_MESSAGE_H_
#define __BOE_MESSAGE_H_

#include <stdint.h>
#include <Header.h>

#define TOKEN_SIZE	128
#define VIDEO_SIZE	1000
#define NACK_NUM	80	


class BoeMessage {
public:
    virtual bool Process(const char *, const BoeHeader *) = 0;
    virtual bool Build(char *) const = 0;

public:    
    BoeMessage() {
    }
    
    virtual ~BoeMessage() {
    }
};

#endif
