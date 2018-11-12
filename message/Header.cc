#include "Header.h"
#include <iostream>
#include <cstdio>
BoeHeader::BoeHeader() {
    version = -1;
    mid = -1;
    uid = 0;
    header_size = 0;
}

BoeHeader::~BoeHeader() {
    version = -1;
    mid = -1;
    uid = 0;
    header_size = 0;
}
/*   
 *   0        8          16                  48
 *   -------------------------------------------
 *    version |  mid     |      uid    
 *   -------------------------------------------
 */

bool BoeHeader::headerEncode(char *src) const {
    src[0] = HEADER_VERSION;
    src[1] = mid;
    char *p = &src[2];
    *(uint32_t *)p = uid;
    return true;
}

bool BoeHeader::headerDecode(char *src) {
    version = src[0];
    mid = src[1];
    uid = *(uint32_t *)&src[2];
    header_size = sizeof(version) + sizeof(mid) + sizeof(uid);
    return true;
}
