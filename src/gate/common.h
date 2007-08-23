#ifndef _COMMON_H
#define _COMMON_H 1

#include <iostream>
#include <string>
#include <sstream>
#include <netinet/in.h>
#include <netdb.h>
#include <stdarg.h>
#include <map>

#define BLOCK_SIZE 1000000

template<class T> struct struct_cmp {
    bool operator() (const T& ip1, const T& ip2) const
    {
        return memcmp(&ip1,&ip2, sizeof (T)) < 0;
    }
};

class Mutex {
    pthread_mutex_t * mutex;
    public:
        Mutex(pthread_mutex_t* m) {
            mutex = m;
            pthread_mutex_lock(mutex);
        }
        ~Mutex() {
            pthread_mutex_unlock(mutex);
        }
        static pthread_mutex_t init() {

            pthread_mutex_t mutex;
            pthread_mutexattr_t mattr;

            pthread_mutexattr_init(&mattr);
            pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE_NP);
            pthread_mutex_init(&mutex, &mattr);
            return mutex;
        }
};
/*
struct cargs_t {
    UDTSOCKET udtsock;
    int       tcpsock;
    int       shutdown;
};
*/

template<class In>
struct Iseq : public std::pair<In, In> {
   Iseq(In i1, In i2): std::pair<In, In>(i1, i2){}
};

template<class C>
Iseq<typename C::iterator_type> iseq(C& c) {
   return Iseq<typename C::iterator_type>(c.begin(), c.end());
};


template<class In, class T>
In find(Iseq<In> r, const T& val) {
   return find(r.first, r.second, val);
};

//
// socks request
//
// 
typedef int TCPSOCKET;

struct sock_pkt {
    char vn;                // version number
    char cd;                // command
    char dstport[2];     // destaiantion port
    char dstip[4];        // destination ip
};

namespace kw {
    static const char * NETWORK = "network";
    static const char * NEIGHBOR = "neighbor";
    static const char * PEER = "peer";

    static const char * UP = "up";
    static const char * DOWN = "down";

    static const char * PEER_PORT = "peer-port";
    static const char * BASE_PORT = "base-port";
    static const char * LOCAL_ADDR = "local-addr";
    static const char * TTL = "ttl";
};

std::string     getString(std::stringstream&);
int             getInt(std::istream);
std::string     itostr(int i);
in_port_t       resolvService (std::string srv);

class Peer;

typedef std::map<struct in_addr,Peer*,struct_cmp<struct in_addr> > peers_map;

extern peers_map peers;

#endif
