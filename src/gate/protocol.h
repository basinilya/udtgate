#ifndef _L_PROTOCOL_H
#define _L_PROTOCOL_H

#include "protocol.h"
/**
 *  @brief The header of protocol message
 * 
*/
struct msg_header {
    uint32_t  bw;
    msg_header () : bw(0) {};
};
/**
 *  @brief  Represent route record data to pass "by wire"
 * 
*/
#pragma pack(1)
struct route_rec {
    uint32_t  prefix;
    uint32_t  mask;
    uint32_t  nexthop;
    uint32_t  m_bw;
    uint32_t  m_hops;
    uint32_t  status;
};
#pragma pack()

#define PEER_MSG_SIZE (100 * sizeof(struct route_rec))

#endif
