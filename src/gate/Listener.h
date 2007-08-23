#ifndef _LISTENER_H_
#define _LISTENER_H_

#include "udtgate.h"
#include "RouteTable.h"

extern RouteTable local_routes;

class Listener {
public:
    //Listener();
protected:
    sockaddr_in  ip4_addr;
    sockaddr_in6 ip6_addr;

    //TCPSOCKET    client_sock;
    //UDTSOCKET    peer_sock; 
    //UDTSOCKET    peer_client_sock; /* for transit connection*/

    static void * Start (void* );
    virtual void Run() {};
private:
};

class LocalListener : Listener {
public:
    LocalListener(int AF);
private:
    TCPSOCKET    sock;
    void LocalListener::Run();
    
};

class PeerListener : public Listener {
public:
    typedef enum{LOCAL, PEER} LSTYPE;

    PeerListener(int AF, LSTYPE type);
protected:
private:
    UDTSOCKET    sock;
    void PeerListener::Run();
};
#endif
