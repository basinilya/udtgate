#ifndef _L_ROUTE_H

#define _L_ROUTE_H	1
#include <string>
#include <sys/types.h>
#include <arpa/inet.h>
#include "protocol.h"

/**
 *  @brief  Represents route record in the system
 * 
*/
class Route {
    uint32_t  prefix;
    uint32_t  mask;
    uint8_t   len;
    void inline _Route(uint32_t p = 0, uint32_t m = 0);
    void inline _Route(uint32_t p = 0, uint8_t m = 0);
public:
    Route(std::string str);
    Route(uint32_t p, uint32_t m);
    Route(uint32_t p, uint8_t l);
    bool match_ip(uint32_t ip);
    int masklen();
    void pack(struct route_rec * r );
};

#endif
