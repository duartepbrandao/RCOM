#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

#include <linux/limits.h>
#include <stdint.h>
#define SENDER 1
#define RECEIVER 2
#define EXIT 3
#define PACKET_MAX_SIZE 65535
#define MAX_STRING_SIZE 10000

#define FLAG 0x7E
#define ADDR_TRANS 0x03
#define ADDR_REC_RESP 0x03
#define ADDR_REC 0x01
#define ADDR_TRANS_RESP 0x01
#define CTRL_SET 0x03
#define CTRL_UA 0x07
#define CTRL_DISC 0x0B

#define FALSE 0
#define TRUE 1

#define NEXT_INDEX(num) (num ^ 0x01)

#define OCTET_ESCAPE 0x7D
#define OCTET_DIFF 0x20

#define MAX_SIZE 256000

#define NEXT_CTRL_INDEX(num) (num << 1)
#define CTRL_REC_READY(num) ((num << 5) | 0x05)
#define CTRL_REC_REJECT(num) ((num << 1) | 0x01)

#define DISCONECTED -50

#endif
