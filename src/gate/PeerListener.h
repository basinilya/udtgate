#ifndef _PEER_LISTENER_H_
#define _PEER_LISTENER_H_

#include "udtgate.h"
#include "RouteTable.h"

extern RouteTable local_routes;

class PeerListener {
public:
    PeerListener();
    void * PeerListener::Start (void* );
    void PeerListener::Run();
private:

};
#endif
