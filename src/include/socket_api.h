#ifndef SOCK_API_H_
#define SOCK_API_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <udt/udt.h>
#include <set>

namespace SOCK_API
{
    typedef enum{TCP, UDT} _proto_t;
    const int UDT_FD_BASE = 0xFFFFFF;

    class FDSET {
    public:
        ::fd_set 	fdset;
        ::UDT::UDSET	udset;

        FDSET ();
        ~FDSET ();
        inline void ZERO();
        inline void SET(UDTSOCKET sd);
        inline void CLR(UDTSOCKET sd);
        inline bool ISSET(UDTSOCKET sd);
    };

    int bind    (UDTSOCKET sd, sockaddr* addr, socklen_t socklen);

    int connect (UDTSOCKET sd, sockaddr* addr, socklen_t socklen);

    int listen  (UDTSOCKET sd, int n);

    int accept  (UDTSOCKET sd, sockaddr* addr, socklen_t* socklenp);

    int send    (UDTSOCKET sd, void *buff, size_t n, int flags);

    int recv    (UDTSOCKET sd, void *buff, size_t n, int flags);

    int recvmsg (UDTSOCKET sd, msghdr *message, int flags);
    int recvmsg (UDTSOCKET sd, const char *buff, size_t n);

    int sendmsg (UDTSOCKET sd, msghdr *message, int flags);
    int sendmsg (UDTSOCKET sd, const char *buff, size_t n);

    int select  (int nfds, FDSET rdfds, FDSET* wfds, FDSET* efds, const struct timeval* timeout);

    int close();
    int shutdown(int how);

    int setsockopt(UDTSOCKET sd, int level, int optname, void* optval, socklen_t optlen);
    int getsockopt(UDTSOCKET sd, int level, int optname, void* optval, socklen_t* optlenp);

};

#endif /*SOCK_API_H_*/
