#include <socket_api.h>
#include <unistd.h>
#include <stdarg.h>
#include <set>
#include <assert.h>
#include <logger.h>
#include <utils.h>


extern int errno;
extern Logger logger;

SOCK_API::FDSET::FDSET() {
    udset.clear();
    FD_ZERO(&fdset);
}
SOCK_API::FDSET::~FDSET() {}

void SOCK_API::FDSET::ZERO() {
    udset.clear();
    FD_ZERO(&fdset);
}
void SOCK_API::FDSET::SET(UDTSOCKET sd) {
    if (sd < UDT_FD_BASE)
        FD_SET(static_cast<int>(sd), &fdset);
    else
        UD_SET(sd-UDT_FD_BASE, &udset);
}
void SOCK_API::FDSET::CLR(UDTSOCKET sd) {
    if (sd < UDT_FD_BASE)
        FD_CLR(static_cast<int>(sd), &fdset);
    else
        UD_CLR(sd-UDT_FD_BASE, &udset);
}
bool SOCK_API::FDSET::ISSET(UDTSOCKET sd) {
    if (sd < UDT_FD_BASE)
        return FD_ISSET(static_cast<int>(sd), &fdset);
    else
        return UD_ISSET(sd-UDT_FD_BASE, &udset);
}

namespace SOCK_API {


    UDTSOCKET socket(int domain, int type, _proto_t proto) {
        if (proto == TCP)
            return ::socket (domain, type, 0);
        else {
            int res = ::UDT::socket(domain, type, 0);
            return res > 0 ? res+UDT_FD_BASE : res;
        }
    }
    int bind(UDTSOCKET sd, sockaddr* addr, socklen_t socklen) {
        if (sd < UDT_FD_BASE)
            return ::bind(static_cast<int>(sd), addr, socklen);
        else
            return ::UDT::bind(sd-UDT_FD_BASE, addr, socklen);
    }
    int connect(UDTSOCKET sd, sockaddr* addr, socklen_t socklen) {
        if (sd < UDT_FD_BASE)
            return ::connect(static_cast<int>(sd), addr, socklen);
        else
            return ::UDT::connect(sd-UDT_FD_BASE, addr, socklen);
    }
    int listen(UDTSOCKET sd, int n) {
        if (sd < UDT_FD_BASE)
            return ::listen(static_cast<int>(sd), n);
        else
            return ::UDT::listen(sd-UDT_FD_BASE, n);
    }
    int accept(UDTSOCKET sd, sockaddr* addr, socklen_t* psocklen) {
        if (sd < UDT_FD_BASE)
            return ::accept(static_cast<int>(sd), addr, psocklen);
        else {
            int int_socklen = static_cast<int>(*psocklen);
            int res = ::UDT::accept(sd-UDT_FD_BASE, addr, &int_socklen);
            *psocklen = static_cast<socklen_t>(int_socklen);
            return res;
        }
    }
    int recv(UDTSOCKET sd, void *buff, size_t n, int flags) {
        if (sd < UDT_FD_BASE)
            ::recv(static_cast<int>(sd), buff, n, flags);
        else {
            int rc = ::UDT::recv(sd-UDT_FD_BASE, static_cast<char*>(buff), n, flags);
            return rc == ::UDT::ERROR ? -1 : rc;
        }
    }
    int send(UDTSOCKET sd, void *buff, size_t n, int flags) {
        if (sd < UDT_FD_BASE)
            return ::send(static_cast<int>(sd), buff, n, flags);
        else {
            int rc =  ::UDT::send(sd-UDT_FD_BASE, static_cast<char*>(buff), n, flags);
            return rc == ::UDT::ERROR ? -1 : rc;
        }
        
    }
    int read(UDTSOCKET sd, void *buff, size_t n, int timeout) {
        if (sd < UDT_FD_BASE)
            return ::read(static_cast<int>(sd), buff, n);
        else {
            int rc = ::UDT::recv(sd-UDT_FD_BASE, static_cast<char*>(buff), n, 0);
            if (rc == ::UDT::ERROR) {
                logger.log_debug(3,"SOCK_API::read: failed: %s\n", 
                        ::UDT::getlasterror().getErrorMessage());
                return -1;
            }
            return rc;
        }
    }
    int readn(UDTSOCKET sd, void *buff, size_t n, int timeout) {
        size_t   nleft;
        ssize_t  nread;
        char   *ptr;
        
        ptr = (char*) buff;
        nleft = n;
        
        while(nleft > 0) {
            if ( (nread = read(sd,ptr,nleft, timeout)) < 0) {
                if(errno == EINTR)
                    nread = 0;
                else
                    return (-1);
            }
            else if (nread == 0)
                break;
            nleft -= nread;
            ptr   += nread;
        }
        return (n-nleft);
    }
    int write(UDTSOCKET sd, void *buff, size_t n, int timeout) {
        if (sd < UDT_FD_BASE)
            return ::write(static_cast<int>(sd), buff, n);
        else {
            int rc = ::UDT::send(sd-UDT_FD_BASE, static_cast<char*>(buff), n, 0);
            return rc == ::UDT::ERROR ? -1 : rc;
        }
    }
    int writen(UDTSOCKET sd, void *buff, size_t n, int timeout) {
        size_t   nleft;
        ssize_t  nwritten;
        char   *ptr;
        
        ptr = (char*) buff;
        nleft = n;
        
        while(nleft > 0) {
            if ( (n = write(sd,ptr,nleft, timeout)) <= 0) {
                if(errno == EINTR)
                    nwritten = 0;
                else
                    return (-1);
            }
            nleft -= nwritten;
            ptr   += nwritten;
        }
        return (n-nleft);
    }
    
    int recvmsg(UDTSOCKET sd, msghdr *message, int flags) {
        if (sd < UDT_FD_BASE)
            return ::recvmsg(static_cast<int>(sd), message, flags);
        else
            return -1; // not proper for UDT
    }
    int recvmsg(UDTSOCKET sd, char *buff, size_t n) {
        if (sd < UDT_FD_BASE)
            return -1; // not proper for TCP
        else {
            int rc = ::UDT::recvmsg(sd-UDT_FD_BASE, buff, n);
            return rc == ::UDT::ERROR ? -1 : rc;
        }
    }
    int sendmsg(UDTSOCKET sd, msghdr *message, int flags) {
        if (sd < UDT_FD_BASE)
            return ::sendmsg(static_cast<int>(sd), message, flags);
        else
            return -1;
    }
    int sendmsg(UDTSOCKET sd, const char *buff, size_t n){
        if (sd < UDT_FD_BASE)
            return -1;
        else {
            int rc = ::UDT::sendmsg(sd-UDT_FD_BASE, buff, n);
            return rc == ::UDT::ERROR ? -1 : rc;
        }
    }
    int close(UDTSOCKET sd){
        if (sd < UDT_FD_BASE)
            ::close(sd);
        else
            ::UDT::close(sd-UDT_FD_BASE);
    }
    int shutdown(UDTSOCKET sd, int how){
        if (sd < UDT_FD_BASE)
            ::shutdown(static_cast<int>(sd), how);
        else
            ::UDT::close(sd-UDT_FD_BASE);
    }
    int setsockopt(UDTSOCKET sd, int level, int optname, void* optval, socklen_t optlen) {
        if (sd < UDT_FD_BASE)
            return setsockopt(sd, level, optname, optval, optlen);
        else
            return ::UDT::setsockopt(sd-UDT_FD_BASE, level, ::UDT::SOCKOPT(optname), optval, optlen);
    }
    int getsockopt(UDTSOCKET sd, int level, int optname, void* optval, socklen_t* optlenp) {
        if (sd < UDT_FD_BASE)
            return ::getsockopt(sd, level, optname, optval,optlenp);
        else {
            if (sizeof(socklen_t) == sizeof(int))
                return ::UDT::getsockopt(sd, level, ::UDT::SOCKOPT(optname), optval, reinterpret_cast<int*>(optlenp));
            else { // safe type "cast" from socklen_t* to int*
                int int_optlen = static_cast<int>(*optlenp);
                int res = ::UDT::getsockopt(sd-UDT_FD_BASE, level, ::UDT::SOCKOPT(optname), optval, &int_optlen);
                *optlenp = static_cast<socklen_t>(int_optlen);
                return res;
            }
        }
    }
    int select(int nfds, FDSET* rfdsp, FDSET* wfdsp, FDSET* efdsp, const struct timeval* timeout) {

        int scale_factor = 1;
        int scale_mask = 3;
        
        FDSET zset, rfds, wfds, efds;
        
        if (! rfdsp)
            rfdsp = &zset;
        if (! wfdsp)
            wfdsp = &zset;
        if (! efdsp)
            efdsp = &zset;
        
        time_t t_min = 100; // start value for internal timeout (us)
        time_t u_timeout = timeout == NULL ? 0 : utl::timeval2utime(timeout);
        time_t time_rest = u_timeout;

        timeval t;
        
        if (timeout != NULL) {
            if (u_timeout < t_min * 2 * scale_mask) {
                t_min =  u_timeout / 2 / scale_mask + 1;
            }
        }
        
        int t_cur = t_min;

        utl::utime2timeval(&t,t_cur);
        //logger.log_debug(3, " SOCK_API::select timeout =  %d / %d \n", t.tv_sec, t.tv_usec);

        while (true) {

            rfds = *rfdsp; 
            wfds = *wfdsp; 
            efds = *efdsp; 
            
            utl::utime2timeval(&t,t_cur);

            //logger.log_debug(3, " SOCK_API::select c_curr =  %d time_rest=%d\n", t_cur, time_rest);
            
            //logger.log_debug(3," ::select ... \n");
            int res1 = ::select(nfds, &rfds.fdset, &wfds.fdset, &efds.fdset, &t);
            //logger.log_debug(3, " ::select = %d\n",res1);

            //logger.log_debug(3," UDT::select ... \n");
            
            int res2 = ::UDT::select(nfds, &rfds.udset, &wfds.udset, &efds.udset, &t);
            
            
            if (res2 == ::UDT::ERROR) 
                logger.log_debug(3," %s\n!!!!!!", ::UDT::getlasterror().getErrorMessage());
                
            //logger.log_debug(3," .. UDT::select  == %d\n", res2);

            time_rest -= t_cur*2 + 1;

            if (res1 > 0 or res2 > 0) { // some data and return;
                * rfdsp = rfds;
                * wfdsp = wfds;
                * efdsp = efds;
                return (res1 > 0 ? res1 : 0) + (res2 > 0 ? res2 : 0);
            }
            else if (res1 < 0 or res2 < 0 or time_rest < 0) { // no data and return
                * rfdsp = * wfdsp = * efdsp = zset;
                if (res1==0 and res2==0)
                    return 0; // time_rest < 0 -- pure timeout
                else
                    return -1; // some error
            }
            
            // no data and continue
            scale_factor = scale_mask & ((scale_factor << 1) + 1);
                
            t_cur = t_min * scale_factor;
            timeval d;

        }
    }
    
    int maxfdn (std::set<UDTSOCKET> sd_set) {
        assert(0);
    }
    int maxfdn (UDTSOCKET sd1, UDTSOCKET sd2) {
        int mfdn = 1;
        if (sd1 < UDT_FD_BASE)
            mfdn = sd1+1;
        if (sd2 < UDT_FD_BASE and sd2 > sd1)
            mfdn = sd2+1;
        return mfdn;
    }
}
