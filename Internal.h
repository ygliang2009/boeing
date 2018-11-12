#ifndef __BOE_INTERNAL_H_
#define __BOE_INTERNAL_H_

#define MIN_BITRATE	80000  		/*10KB*/
#define MAX_BITRATE	1600000 	/*2MB*/
#define START_BITRATE	800000		/*100KB*/

#define DEFAULT_PACKET_SIZE	1024

#define ACK_REAL_TIME		20
#define ACK_HB_TIME		200

#define MTU			1000
#define MAX_PACKET_SIZE		1000

#define DEFAULT_BITRATECTRL_MINBITRATE	10000
#define DEFAULT_BITRATECTRL_MAXBITRATE	1500000

/*清理资源*/
#define CLEANUP(x_) \
do { \
    if ((x_) != NULL) \
	delete (x_); \
    (x_) = NULL; \
} while(0)

#endif
