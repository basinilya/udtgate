#include "Peer.h"


in_port_t Peer::base_port = 9000;

Peer::Peer(std::string addr, int port) {
    _Peer(addr, base_port);
    //__DEBUG__(peer_addr + ":" + peer_port);
};
Peer::Peer(std::string addr) {
    _Peer(addr, base_port);
};

Peer::Peer(struct in_addr ip) {
    std::string addr = std::string(inet_ntoa(ip));
    _Peer(addr, base_port);
};

Peer::~Peer () {
    delete contr_buf_in;
    delete contr_buf_out;
}
void Peer::_Peer(std::string addr, int port) {
    AF = AF_INET;
    m_status = IDLE;
    _shutdown = false;
    peer_addr = addr;
    peer_port = port;
    local_addr = "0.0.0.0";
    contr_buf_in  = new char[PEER_MSG_SIZE];
    contr_buf_out = new char[PEER_MSG_SIZE];
};

in_port_t Peer::getDefaultPort() {
    return base_port;
}

void Peer::setDefaultPort (in_port_t port) {
    base_port = port;
}

void Peer::setPeerPort(in_port_t port) {
    peer_port = port;
}
in_port_t Peer::getPeerPort() {
    return peer_port;
}
void Peer::setPeerAddr(std::string& addr) {
    peer_addr = addr;
}
void Peer::setLocalAddr(std::string& addr) {
    local_addr = addr;
}
void Peer::setShutdown(const bool shut) {
    _shutdown = shut;
}
void Peer::setTTL(const int t) {
    ttl = t;
}
void Peer::Init () {

    memset(&hints, 0, sizeof(struct addrinfo));
    std::cout << "init: " << local_addr << ":" << base_port << "\t"<< peer_addr << ":" << peer_port << std::endl;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_DGRAM;
 
    if (AF == AF_INET6)
        hints.ai_family = AF_INET6;
    else
        hints.ai_family = AF = AF_INET;
 
    if (0 != getaddrinfo(peer_addr.c_str(), NULL, &hints, &peeraddr)) {
        log_tagmsg(this,LOG_ERR,LOG_IGN, "incorrect peer network address: %s:%d\n", peer_addr.c_str(),peer_port);
        return;
    }
    if (0 != getaddrinfo(local_addr.c_str(), NULL, &hints, &localaddr)) {
        log_tagmsg(this,LOG_ERR,LOG_IGN, "incorrect local network address: %s:%d\n", local_addr.c_str(), base_port);
        return;
    }
    typedef sockaddr_in* sockaddr_in_p;
    typedef sockaddr_in6* sockaddr_in6_p;
    if ( AF == AF_INET6) {
        reinterpret_cast<sockaddr_in6_p>(localaddr->ai_addr)->sin6_port = htons(base_port);
        reinterpret_cast<sockaddr_in6_p>(peeraddr->ai_addr)->sin6_port = htons(peer_port);
    }
    else {
        reinterpret_cast<sockaddr_in_p> (localaddr->ai_addr)->sin_port = htons(base_port);
        reinterpret_cast<sockaddr_in_p> (peeraddr->ai_addr)->sin_port = htons(peer_port);
    }

    pthread_t th;
    pthread_create(&th,NULL, Start, this);

}

void* Peer::Start (void * ar) {

    Peer *p = (Peer*) ar;
    p->Run();
    return NULL;
}

void Peer::Run() {
    while (true) {

        m_status = CONNECTING;

        peersock = UDT::socket(peeraddr->ai_family, SOCK_DGRAM, peeraddr->ai_protocol);

        if (UDT::bind(peersock, localaddr->ai_addr, localaddr->ai_addrlen) == UDT::ERROR ) {
                log_tagmsg(this,LOG_ERR, LOG_IGN, "bind error\n");
                sleep(60);
                continue;
        }

        UDT::setsockopt(peersock, 0, UDT_RCVTIMEO, new int(1000), sizeof(int));
        UDT::setsockopt(peersock, 0, UDT_RENDEZVOUS, new int(1), sizeof(int));

        __DEBUG__("-");

        while(true) {
            if (UDT::connect(peersock, peeraddr->ai_addr,peeraddr->ai_addrlen)) {
                log_tagmsg(this,LOG_ERR, LOG_IGN,"%s\n", UDT::getlasterror().getErrorMessage());
                log_tagmsg(this, LOG_MSG, LOG_IGN, "connect: pending...\n");
                sleep(1);
                continue;
            }
            last_recv_time = time(NULL);
            last_send_time = 0;
            m_status = CONNECTED;
            break;
        }
        log_tagmsg(this, LOG_MSG, LOG_IGN, "connected.\n");
        try { 
            while (true) {
                int sz;
                time_t t = time(NULL);

                if (t > (last_send_time + keepalive)) {
                    send_msg();
                    last_send_time = time(NULL);
                }
                //__DEBUG__("==> rcv");
                if(recv_msg())
                    last_recv_time = time(NULL);
                //__DEBUG__("<== rcv");
                if (t > (last_recv_time + keepalive*3))
                    throw(std::string("rcv timeout"));
            }
        }
        catch (std::string e) {
            log_tagmsg(this, LOG_ERR, LOG_IGN, "%s\n", e.c_str());
        }
        catch (...) {
            //__DEBUG__("catch exception ???");
        }
        __DEBUG__("- >>");
        UDT::close(peersock);
        m_status = CLOSED;
        __DEBUG__("- <<");
    }
}

std::list<Route> Peer::getRoutes() {
    return route_table.getRoutes();
}
Route* Peer::Lookup(uint32_t ip) {
    return  route_table.lookup(ip);
}
pthread_mutex_t * Peer::getRouteMutex () {
    return &route_table.mutex;
}
void Peer::send_msg () {

    memset(contr_buf_out,sizeof(msg_header),0);

    //memcpy(contr_buf_out, new msg_header, sizeof(msg_header));

    int sz = local_routes.pack(contr_buf_out+sizeof(msg_header),PEER_MSG_SIZE-sizeof(msg_header));
    int ss = UDT::sendmsg(peersock, contr_buf_out, sz+sizeof(msg_header));

    if (ss) {
        if(ss == UDT::ERROR)
            throw(std::string("send error"));
        else
            last_send_time = time(NULL);
        __DEBUG__("sent " << ss << " bytes of " <<sz+sizeof(msg_header) << " implied");
    }
    else {
        __CROAK__("error");
    }
}
bool Peer::recv_msg () {


    //__DEBUG__("==> recvmsg");
    int sz = UDT::recvmsg(peersock, contr_buf_in, PEER_MSG_SIZE);
    if (sz) {
        if(sz == UDT::ERROR) 
            throw(std::string("rcv error"));
        else {
            log_tagmsg(this, LOG_MSG, LOG_IGN, "get route data\n");
            Mutex m (&route_table.mutex);
            if((sz -= sizeof(msg_header)) < 0)
                throw(std::string("peer message format error 1"));

            for (struct route_rec * data_ptr = (struct route_rec*) (contr_buf_in+sizeof(msg_header)); sz>0; data_ptr++) {
                if ((sz -=sizeof(struct route_rec)) < 0)
                    throw(std::string("peer message format error 2"));
                    route_table.add( *(new Route(ntohl(data_ptr->prefix), htonl((data_ptr)->mask))));
            };

        }
        return true;
    }

    return false;
}
in_addr     Peer::getPeerAddrStr() {
    return ((sockaddr_in*) (peeraddr->ai_addr))->sin_addr;
}

std::string Peer::tag () {
    return std::string("neighbor ") + peer_addr+ ":" + itostr(peer_port);
}

bool Peer::isConnected () {
    return m_status == CONNECTED;
}

Peer::STATUS Peer::getStatus() {
    return m_status;
}
