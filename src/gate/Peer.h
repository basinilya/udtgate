#ifndef __PEER_H
#define __PEER_H	1

#include <string>
#include <list>
#include <map>
#include <udt.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "protocol.h"
#include "common.h"
#include "RouteTable.h"

extern RouteTable local_routes;

class Peer : public Tag {

    void _Peer(const std::string addr, int port);
public:
    typedef enum {
        IDLE,
        CONNECTING,
        CONNECTED,
        FATALERROR,
        CLOSED
    } STATUS;

    STATUS getStatus();
    Peer(const std::string addr, int port);
    Peer(const std::string addr);
    Peer(const struct in_addr);
    ~Peer();

    void                     Init();
    bool                     isConnected();
    UDTSOCKET       getDataConnection(std::string&);
    std::list<Route>   getRoutes();

    void setPeerAddr (std::string& addr);
    void setLocalAddr(std::string& addr);
    std::string getPeerAddr();
    std::string getLocalAddr();
    in_addr     getPeerAddrStr();

    void setPeerPort(in_port_t port);
    in_port_t getPeerPort();

    void setShutdown(const bool shut);
    void setTTL(const int ttl);

    static void         setDefaultPort(in_port_t port);
    static in_port_t    getDefaultPort();
    static void         setDefaultTTL(const int ttl);
    std::string         tag();

private:
    int             AF;
    std::string  peer_addr;
    in_port_t   peer_port;
    std::string  local_addr;
    int ttl;
    time_t last_send_time;
    time_t last_recv_time;
    STATUS m_status;
    bool _shutdown;

    RouteTable route_table;
    addrinfo *  peeraddr;
    addrinfo *  localaddr;

    char *contr_buf_in;
    char *contr_buf_out;

    UDTSOCKET   peersock;
    addrinfo    hints;

    static in_port_t base_port;
    static int default_ttl;
    static const int  keepalive = 3;

    static void* Start(void*);
    void Run();
    int getDataConnection(void);
    Route* Lookup(uint32_t ip);
    pthread_mutex_t * getRouteMutex();
    void send_msg ();
    inline bool recv_msg ();
};

#endif
