#ifndef _UDTGATE_H
#define _UDTGATE_H

#ifdef  DEFAULT_INCLUDES
#  ifndef Win32
#    include <errno.h>
#    include <config.h>
#    include <unistd.h>
#    include <sys/socket.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <arpa/inet.h>
#    include <signal.h>
#    include <poll.h>
#    include <sys/types.h>
#    include <sys/sysctl.h>
#    include <syslog.h>
#  else // Win32
#    include <winsock2.h>
#    include <Ws2tcpip.h>
#  endif
#
#  include <cstdlib>
#  include <cstring>
#  include <iostream>
#  include <errno.h>
#  include <assert.h>
#  include <stdarg.h>
#  include <udt/udt.h>
#  include <logger.h>
#endif  // COMMON_INCLUDE

#include <udt/udt.h>
#include <map>
#include <set>

const int MAX_NAME_SZ = 128;    // buffer size for addr/port/other names
const int BLOCK_SIZE  = 1024*1024; // buffer size for IO operations

struct cargs_t {
    UDTSOCKET udtsock;
    int       tcpsock;
    int       shutdown;
};


class globals {
public:
    static int  net_access; // network access 0 - loopback; 1 - local subnets; 2+ - any.
    static int  debug_level;
    static int  dump_message;
    static bool track_connections;
    static bool rendezvous;
    static bool demonize;
#ifdef UDP_BASEPORT_OPTION
    static int baseport;
    static int maxport;
#endif
    static char * sock_ident;
    static char * peer_ident;

    static char * app_ident;
    static char * serv_ident;

};

#pragma pack(1)
struct sock_pkt {
    char vn;
    char cd;
    char dstport[2];
    char dstip[4];
};
#pragma pack()

/*
template <class T> class SET {
	set<T> data;
public:
	void drop(T e) 		{data.erase(e);};
	bool isset(T e) 	{return data.find(e) != data.end();};
	void set(T e)   	{data.insert(e);}
	void clear()  		{data.clear();}
};
*/

#endif
