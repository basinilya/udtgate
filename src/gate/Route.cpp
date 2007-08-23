#include <string>
#include <memory>
#include "Route.h"

Route::Route(std::string str) {
    uint32_t addr, mask, a[4], l, pos;
    char * s = new char [str.length()];
    std::auto_ptr<char> sp(s);

    try {
        if (sscanf(str.c_str(),"%u.%u.%u.%u%n%s",&a[3],&a[2],&a[1],&a[0],&pos,s) == 6) {
            for (int i=0,k=1; i<4; i++,k*=256) {
                if (a[i] > 255)
                    throw (std::string(""));
                addr += a[i]*k;
            }
            if (strlen(s) == 0) {
                l = a[3] >= 224 ? 0 : 
                    a[3] < 128 ? 8 :
                    a[3] < 192 ? 16 : 24;
                _Route(addr, (uint8_t) l);
                return;
            } else {
                if (sscanf(str.c_str()+pos,"/%u.%u.%u.%u%s",&a[3],&a[2],&a[1],&a[0],s) == 4) {
                    for (int i=0,k=1; i<4; i++,k*=256) {
                        if (a[i] > 255)
                            throw (std::string(""));
                        mask += a[i]*k;
                    }
                    _Route(addr, mask);
                } else if (sscanf(str.c_str()+pos,"/%u%s",&l,s) == 1) {
                    if (l>32)
                        throw (std::string(""));
                    _Route(addr, (uint8_t) l);
                } else
                    throw (std::string(""));
            }
        }
    } catch (std::string e) {
        throw (std::string("wrog prefix"));
    }
}
Route::Route(uint32_t p, uint32_t m) {
    _Route(p,m);
}
Route::Route(uint32_t p, uint8_t l) {
    _Route(p,l);
}
void Route::_Route(uint32_t p, uint32_t m) {

    if (p & (!m))
        throw("bad ip: check mask error");

    prefix = p;
    mask = m;

    for (len=0; m != 0; m>>=1)
        if (m & 01) len++;
}
void Route::_Route(uint32_t p, uint8_t l) {
    uint32_t m = htonl((0xFFFFFFFF >> l) << l);
    _Route(p,m);
}
bool Route::match_ip(uint32_t ip) {
    return(ip & mask ==  prefix);
}
int Route::masklen() {
    return(int) len;
}

void Route::pack(struct route_rec * r ) {
    r->prefix    = htonl(prefix);
    r->mask      = htonl(mask);
    r->nexthop   = 0;
    r->m_bw      = 0;
    r->m_hops    = 0;
}
