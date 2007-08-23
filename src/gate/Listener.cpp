#include "common.h"
#include "RouteTable.h"
#include "Peer.h"
#include "Worker.h"
#include "Listener.h"

 
void* Listener::Start (void* ar) {
    Listener *p = (Listener*) ar;
    p->Run();
    return NULL;
}

PeerListener::PeerListener (int AF, LSTYPE lstype)
{   
    int ERROR = lstype == PEER ? UDT::ERROR : -1;
    int status;

    if(AF == AF_INET) {
        sockaddr_in peer_addr;
        int       tcp_lsock;
        UDTSOCKET udt_lsock;
        size_t    tcp_socklen;
        int       udt_socklen;

        void* socklenref;
        void* lsockref;

        lsockref = (lstype == PEER ? &udt_lsock : &tcp_lsock);
        lsockref = (lstype == PEER ? &udt_lsock : &tcp_lsock);

        if (lstype == PEER) {
            lsockref   = &udt_lsock;
            socklenref = &udt_socklen;
            *(int*)socklenref = sizeof(sockaddr_in);

            * (UDTSOCKET*) lsockref = UDT::socket(AF, SOCK_STREAM, 0);
                
        } else {
            lsockref   = &tcp_lsock;
            socklenref = &tcp_socklen;
            *(socklen_t*) socklenref = sizeof(sockaddr_in);

            * (int*) lsockref = socket(AF_INET, SOCK_STREAM, 0);
        }

        ip4_addr.sin_port = Peer::getDefaultPort();
        ip4_addr.sin_addr.s_addr = INADDR_ANY;

        // binding loop
        while (true) {
            if(lstype == PEER) {
                status = UDT::bind(*(UDTSOCKET*)lsockref, (sockaddr*) &ip4_addr, sizeof(sockaddr_in));
            }
            else {
                status = bind(*(int*)lsockref, (sockaddr*) &ip4_addr, sizeof(sockaddr_in));
            }
            if ( status == ERROR) {
                log_msg(LOG_ERR, LOG_IGN, "cannot bind port: %d stil trying ...\n", ntohs(ip4_addr.sin_port));
                sleep(60);
            };
        }

        if (lstype == PEER) 
            UDT::listen(*(UDTSOCKET*)lsockref,10);
        else 
            listen(*(int*)lsockref,10);

        while (true) {
            int       tcp_asock;
            UDTSOCKET udt_asock;
            void * asockref;
            // currently UDTSOCKET is int but we can not guarantee It!

            asockref = (lstype == PEER ? &udt_asock : &tcp_asock);
            
            if (lstype == PEER) {
                status = * (UDTSOCKET*) asockref = UDT::accept(*(UDTSOCKET*)lsockref, (sockaddr*) &peer_addr, (int*) socklenref);
            } else {
                status = * (int*) asockref = accept(*(int*)lsockref, (sockaddr*) &peer_addr, (socklen_t*) socklenref);
            }
            if (status == ERROR) {
                log_msg(LOG_ERR, LOG_IGN, "accept error...");
                sleep(10);
                continue;
            }
            // authorization
            bool pass = false;
            peers_map::iterator it;
            for( it = peers.begin(); it != peers.end(); it++) {
                if (it->second->getPeerAddrStr().s_addr == peer_addr.sin_addr.s_addr and
                    it->second->getStatus() == Peer::CONNECTED) {
                    pass = true;
                    break;
                }
            };
            if (pass) {
                if (true) {
                    InWorker * w = new InWorker(*(int*)asockref);
                }
            }
            else {
                //!!!
                char * ip =  inet_ntoa(peer_addr.sin_addr);
                log_msg(LOG_ERR, LOG_IGN, "peer autorization error\n");
                UDT::close(sock);
            }
        }
    }
    else {
        __CROAK__("IPv6 not realised yet");
    }
}
void PeerListener::Run() {

}
LocalListener::LocalListener (int AF)
{

}
void LocalListener::Run() {

}
