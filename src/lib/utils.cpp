#include <config.h>
#include <unistd.h>
#include <logger.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <utils.h>
#include <errno.h>

extern Logger logger;
extern int debug_level;


namespace utl {
    using namespace std;
    char* dump_inetaddr(sockaddr_in * iaddr, char * buf, bool port) {
    	if (port)
    	    sprintf(buf,"%s:%d",inet_ntoa(iaddr->sin_addr),
                ntohs(iaddr->sin_port));
    	else
    	    sprintf(buf,"%s",inet_ntoa(iaddr->sin_addr));
    		
        return buf;
    }
    
    string get_string (stringstream& is) {
        string s; is >> s; return s;
    }
    int get_int (istream is) {
        int i;  is >> i; return i;
    }
    string i2str(int i) {
        stringstream ss("");
        ss << i;
        return ss.str();
    }
    ////
     // resolv service name and return port number in host order
     ///
    in_port_t resolvService (string srv) {
        in_port_t port;
        if (port = atoi(srv.c_str()))
            return port;
        else {
            struct servent* sent;
            if (sent = getservbyname(srv.c_str(),  "udp")) {
                return ntohs(sent->s_port);
            }
        }
        throw(string("wrong service or port:")+srv);
    }
    ////
     // Validate network address and return address family
     ///
    int resolvAddress (string addr) {
        __CROAK__("not implmented yet");
        return 1;
    }

    ////
     //Check if the address match local interfaces or subnets
     ///
    bool check_source (sockaddr * addr, bool subnet) {

        char addr_str[256];
        char * where = "check_source:";
        int af;

        af = addr->sa_family;

        dump_inetaddr((sockaddr_in*) addr, addr_str);
       
#if HAVE_GETIFADDRS
        struct ifaddrs *ifp, *ifap;
        if(::getifaddrs(&ifap)<0)
            logger.log_die("getifaddrs failed: errno = %d\n", errno);
        //printf("getifaddrs = %d\n", ifap);

        for(ifp = ifap; ifp != NULL; ifp = ifp->ifa_next) {
            struct sockaddr addr1;

            //printf("matching against = %s\n", ifp->ifa_name);

            if (ifp->ifa_addr == NULL or ifp->ifa_netmask == NULL)
                continue;

            memcpy(&addr1, addr, sizeof(addr1));

            if(af != ifp->ifa_addr->sa_family)
                continue;

            if (subnet) {
                sockaddr_mask(&addr1,ifp->ifa_netmask);
                sockaddr_mask(ifp->ifa_addr,ifp->ifa_netmask);
            }
            
            if (logger.getDebugLevel()) {
                char addr1_str[256], ifaddr_str[256];
                dump_inetaddr((sockaddr_in*) &addr1, addr1_str);
                dump_inetaddr((sockaddr_in*) ifp->ifa_addr, ifaddr_str);
                
                logger.log_debug("%s matching %s against %s -> %s\n",where, addr1_str, ifp->ifa_name, ifaddr_str);
            }
            if(sockaddr_match(&addr1,ifp->ifa_addr)) {
            	logger.log_debug("%s connecting %s matched against %s\n", 
            			where, addr_str,ifp->ifa_name);
                ::freeifaddrs(ifap);
                return true;
            }
        }
        logger.log_debug("%s no match\n", where);
        
        ::freeifaddrs(ifap);
#else
#  error getifaddr no relized!
#endif // HAVE_GETIFADDRS
        return false;
    }
    bool sockaddr_match(sockaddr * addr1, sockaddr * addr2, bool portcmp) {

        assert(addr1->sa_family == addr2->sa_family
               && (addr1->sa_family == AF_INET || addr1->sa_family == AF_INET6));

        switch(addr1->sa_family) {
        case AF_INET:
            if (portcmp and ((sockaddr_in*) addr1)->sin_port != ((sockaddr_in*) addr1)->sin_port)
                return false;
            return memcmp(
                          & reinterpret_cast<sockaddr_in*>(addr1)->sin_addr,
                          & reinterpret_cast<sockaddr_in*>(addr2)->sin_addr,
                          sizeof (reinterpret_cast<sockaddr_in*>(addr1)->sin_addr)
                         ) ? false : true;
        case AF_INET6:
            if (portcmp and ((sockaddr_in6*) addr1)->sin6_port != ((sockaddr_in6*) addr1)->sin6_port)
                return false;
            return memcmp(
                          & reinterpret_cast<sockaddr_in6*>(addr1)->sin6_addr,
                          & reinterpret_cast<sockaddr_in6*>(addr2)->sin6_addr,
                          sizeof (reinterpret_cast<sockaddr_in6*>(addr1)->sin6_addr)
                         ) ? false : true;
        }
        return false;
    }
    void sockaddr_mask(sockaddr * addr1, sockaddr * netmask) {

        assert(addr1->sa_family == netmask->sa_family
               && (addr1->sa_family == AF_INET || addr1->sa_family == AF_INET6));

        switch(addr1->sa_family) {
        case AF_INET:
            memand(
                   & reinterpret_cast<sockaddr_in*>(addr1)->sin_addr,
                   & reinterpret_cast<sockaddr_in*>(netmask)->sin_addr,
                   sizeof (reinterpret_cast<sockaddr_in*>(addr1)->sin_addr)
                  );
            return;
        case AF_INET6:
            memand(
                   & reinterpret_cast<sockaddr_in6*>(addr1)->sin6_addr,
                   & reinterpret_cast<sockaddr_in6*>(netmask)->sin6_addr,
                   sizeof (reinterpret_cast<sockaddr_in6*>(addr1)->sin6_addr)
                  );
            return;
        }
    }
    void memand (void * dst, void * src, size_t size) {
    	char * dst1, *src1;

        for(dst1 = (char*) dst, src1 = (char*) src; size>0; size--, dst1++, src1++)
            *dst1 &= *src1;
    }
}

/*
 struct ifconf ifc;
 struct ifreq * ifr;
 int ifn; // number of interfaces
 int ifstep = 16; // incremental step
 int af;


 getifaddrs();

 printf("addr = %s\n", buf);

 af = addr->sa_family;

 ifc.ifc_buf = NULL;


 for (ifn = ifstep, ifc.ifc_len = ifn*sizeof(ifreq); ifc.ifc_len == ifn * sizeof(ifreq); ifn +=ifstep) {
 ifc.ifc_buf = (char*) realloc (ifc.ifc_buf, ifn * sizeof(ifreq));
 ifc.ifc_len = ifn * sizeof(ifreq);
 if (ioctl(fd,SIOCGIFCONF, &ifc) < 0) {
 perror("check_source: ioctl");
 free(ifc.ifc_buf);
 return false;
 }
 }
 printf(" ifn = %d\n", ifc.ifc_len / sizeof(ifreq));

 getifaddrs();

 for (ifr = (ifreq *) ifc.ifc_buf; ifr < (ifreq *) (ifc.ifc_buf + ifc.ifc_len); ifr++)
 printf("1:>>> %s\n", ifr->ifr_name);

 for (ifr = (ifreq *) ifc.ifc_buf; ifr < (ifreq *) (ifc.ifc_buf + ifc.ifc_len); ifr++) {

 printf("2:>>> %s\n", ifr->ifr_name);

 char unsigned abuf[16];
 char unsigned ibuf[16];
 char unsigned mbuf[16];

 ifreq ifa, ifm;

 ifa = ifm = *ifr;

 if (ioctl(fd, SIOCGIFADDR, &ifa) < 0) {
 perror("");
 logger.log_err("%s ioctl/SIOCGIFADDR failed\n", where);
 free(ifc.ifc_buf);
 return false;
 }
 if (ioctl(fd, SIOCGIFNETMASK, &ifm) < 0) {
 logger.log_err("%s ioctl/SIOCGINETFMASK failed\n", where);
 free(ifc.ifc_buf);
 return false;
 }

 int  alen;

 switch (af) {
 case AF_INET:
 alen =  sizeof((reinterpret_cast<sockaddr_in*>(addr))->sin_addr);
 memcpy(abuf, & (reinterpret_cast<sockaddr_in*>(addr))->sin_addr, alen);
 memcpy(ibuf, & (reinterpret_cast<sockaddr_in*>(&ifa.ifr_addr))->sin_addr, alen);
 memcpy(mbuf, & (reinterpret_cast<sockaddr_in*>(&ifm.ifr_addr))->sin_addr, alen);
 break;
 case AF_INET6:
 alen =  sizeof((reinterpret_cast<sockaddr_in6*>(addr))->sin6_addr);
 memcpy(abuf, & (reinterpret_cast<sockaddr_in6*>(addr))->sin6_addr, alen);
 memcpy(ibuf, & (reinterpret_cast<sockaddr_in6*>(&ifa.ifr_addr))->sin6_addr, alen);
 memcpy(mbuf, & (reinterpret_cast<sockaddr_in6*>(&ifm.ifr_addr))->sin6_addr, alen);
 break;
 default:
 logger.log_die("%s wrong address family: %d\n", where, af);
 }

 //printf("mbuf: %d.%d.%d.%d\n", mbuf[0], mbuf[1],  mbuf[2],  mbuf[3]);
 //printf("ibuf: %d.%d.%d.%d\n", ibuf[0], ibuf[1],  ibuf[2],  ibuf[3]);
 //printf("abuf: %d.%d.%d.%d\n", abuf[0], abuf[1],  abuf[2],  abuf[3]);
 //printf("---\n");

 if (subnet) for(int i=0; i<alen; i++) { // apply mask in the "subnet" mode;
 abuf[i] &= mbuf[i];
 ibuf[i] &= mbuf[i];
 }
 //printf("mbuf: %d.%d.%d.%d\n", mbuf[0], mbuf[1],  mbuf[2],  mbuf[3]);
 //printf("ibuf: %d.%d.%d.%d\n", ibuf[0], ibuf[1],  ibuf[2],  ibuf[3]);
 //printf("abuf: %d.%d.%d.%d\n", abuf[0], abuf[1],  abuf[2],  abuf[3]);

 if (!memcmp(abuf, ibuf, alen)) {
 free(ifc.ifc_buf);
 return true;
 }
 }

 free(ifc.ifc_buf);
 */
