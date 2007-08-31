#include <sys/wait.h>
#include <signal.h>
#include <udt/udt.h>
#include <udt/cc.h>
#include <setproctitle.h>
#define  DEFAULT_INCLUDES
#include <udtgate.h>
#include <utils.h>
#include <stdlib.h>
#include "udtrelay.h"

using namespace std;

void* start_child(void*);

/**
 *  Worker for traffering data from sock to peer sockets
 */
void* sock2peer_worker(void*);

/**
 *  Worker for traffering data from peer to sock sockets
 */
void* peer2sock_worker(void*);

/**
 *  Set default options for UDT sockets
 */
void  setsockopt(UDTSOCKET);

bool  check_udp_buffer(UDTSOCKET,UDTOpt);

int   parse_udt_option (char * optstr);

bool  is_custom_value(UDTOpt optcode);

/**
 *  Check address against local interfaces/subnets
 *  returns true if match
 */
bool check_source (sockaddr* addr, bool subnet);

void exit_handler(int);

// GLOBAL SETTINGS
int  net_access = 0; // network access 0 - loopback; 1 - local subnets; 2+ - any.
int  debug_level = 0;
bool track_connections = false;
bool rendezvous = false;
bool demonize = false;
// GLOBAL SETTINGS

char * app_ident   = "udtrelay";
char * sock_ident =  "sock2peer server";
char * peer_ident  = "peer2sock server";
char * serv_ident  = app_ident;

int PID;
char sPeer_addr[MAX_NAME_SZ+1] = "";
char sPeer_listen_port[MAX_NAME_SZ+1] = "";
char sPeer_remote_port[MAX_NAME_SZ+1] = "";
char sSocks_port[MAX_NAME_SZ+1] = "";

char sPeer_rzv_lport[MAX_NAME_SZ+1] = "";
char sPeer_rzv_rport[MAX_NAME_SZ+1] = "";

addrinfo hints;

addrinfo *pPeeraddr = NULL; // remote UDT peer address/port
addrinfo *pServaddr = NULL; // local  UDT peer address/port

addrinfo *pPeerRzvLocal  = NULL; // UDT client allocates it in rzv node
addrinfo *pPeerRzvRemote = NULL; // UDT server connects to it in rzv mode


enum {DUAL,SERVER,CLIENT} mode  = DUAL;

#ifdef UDP_BASEPORT_OPTION
int baseport = 0;
int maxport = 0;
#endif


int rcvbuffer = BLOCK_SIZE;

typedef struct {
    char * optname;
    UDTOpt	   optcode;
} UDT_OPTION_T;

UDT_OPTION_T UDT_OPTIONS_LIST [] = {
    {"UDT_MSS",		UDT_MSS},
    {"UDT_RCVBUF",	UDT_RCVBUF},
    {"UDP_RCVBUF",	UDP_RCVBUF},
    {"UDT_SNDBUF",	UDT_SNDBUF},
    {"UDP_SNDBUF",	UDP_SNDBUF},
    {"UDT_LINGER",	UDT_LINGER}
};

typedef map<UDTOpt, int> UDT_OPTIONS_T;

UDT_OPTIONS_T udt_options;

char* cc_lib[] = {"Vegas","TCP","ScalableTCP","HSTCP","BiCTCP", "Westwood", "FAST"};
char* ccc = "";

extern char *optarg;
extern int   optind, opterr;

Logger logger("udtrelay");

int main(int argc, char* argv[], char* envp[])
{

    int c;
    static char optstring [] = "hdDNLCSP:R:B:c:U:";
    opterr=0;
    UDTSOCKET ludtsock;
    int       ltcpsock = 0;

    logger.setInteractive(true);

    char *usage = \
        "\nusage:\n"
        "\n"
        "udtgate [OPTIONS] <socks_port> <peer_port> <peer_addr[:port]>  \n"
        "\n"
        "  <socks_port>       TCP ports to listen for incomming client socks connections.\n"
        "  <peer_port>        UDT/UDP port to use for incomming peer UDT connections.\n"
        "                     The same peer port  number is used for outgoing UDT\n"
        "                     connections by default\n"
        "\n"
        "  <peer_addr[:port]> Remote UDT peer address and optional custom remote port.\n"
        "\n"
        "  OPTIONS: \n"
        "    -h               How this help and exit.\n"
        "    -d               Encrease debug level.\n"
        "    -D               Demonize.\n"
        "    -L               Log connections (by default - in the debug mode)\n"
        "    -N               Allow socks connections from attached subnets \n"
        "                     (by default only internal connections are permited);\n"
        "                     appling this option twice - allows all incoming\n"
        "                     coonections.\n"
        "    -C               Client-only mode: don't accept incoming peer/UDT\n"
        "                     connections.\n"
        "    -S               Server-only mode: don't accept outgoing socks\n"
        "                     connections.\n"
        "    -R <lp>[:<rp>]   Turn the rendezvous mode on.\n"
        "                     * UDT peer client allocates <lp> port and tries\n"
        "                       to connect to the servers's <peer_port>\n"
        "                     * UDT peer server allocates <peer_port> and tries\n"
        "                       to connect to the clinet's <rp> port which is \n"
        "                       by default equals to <lp>.\n"

#ifdef UDP_BASEPORT_OPTION
        "    -P <from:to>     UDP port range to use for UDT data chanel.\n"
#endif
        "    -B <rcv>         (-)UDT rcv buffer size in megabytes (default: 1Mb)\n"
        "    -c <ccc>         Congetion control class:\n"
        "                       UDT (default), TCP, Vegas, ScalableTCP, HSTCP,\n"
        "                       BiCTCP, Westwood, FAST.\n"
        "    -U <opt=val>     Set some additional UDT options for UDT socket:\n"
        "                       UDT_MSS, UDT_RCVBUF, UDT_SNDBUF, UDP_RCVBUF or\n"
        "                       UDP_SNDBUF.\n"
        "                     Quantifiers K(ilo) and M(ega) are accepted as \n"
        "                     suffixes.\n"
        "\n"
        "  Options marked with (-) have not been yet implemented."
        "\n";

    while ((c=getopt(argc, argv, optstring)) != -1) {
        switch (c) {
        case 'h':
            logger.log_info("%s", usage);
            exit(0);
        case 'N':
            net_access++;
            break;
        case 'd':
            debug_level++;
            break;
        case 'D':
            demonize = true;
            break;
        case 'L':
            track_connections = true;
            break;
        case 'C':
            if (mode != DUAL)
                logger.log_die(" -C option can not be used with -S.\n");
            mode = CLIENT;
            break;
        case 'S':
            mode = SERVER;
            if (mode != DUAL)
                logger.log_die(" -S option can not be used with -C.\n");
            break;
        case 'R':
            rendezvous = true;
            int c;
            if((c = sscanf(optarg,"%[^:]:%[^:]",sPeer_rzv_lport,sPeer_rzv_rport)) < 1)
                logger.log_die("Wrong -R option syntax.\n");
            if (c = 1)
                strcpy(sPeer_rzv_rport,sPeer_rzv_lport);
            break;
        case 'B':
            rcvbuffer = atoi(optarg);
            if (rcvbuffer == 0)
                logger.log_die("Wrong -B option value\n%s", usage);
            rcvbuffer *= 1000000;
            break;
#ifdef UDP_BASEPORT_OPTION
        case 'P':
            if(sscanf(optarg,"%d:%d",&baseport,&maxport) != 2)
                logger.log_die("Wrong -P option syntax.\n");
            if (baseport < 1024 || baseport > 0xFFFE ||
                maxport  < 1024 || maxport  > 0xFFFE || maxport <= baseport)
                logger.log_die("Wrong -P option values: %s.\n", optarg);
            break;
#endif
        case 'c':
            ccc = (char*) malloc(strlen(optarg)+1);
            strcpy(ccc, optarg);
            {
                bool ccc_ok = false;
                for (int i=0; i< sizeof(cc_lib)/sizeof(*cc_lib); ++i ) {
                    if (! strcasecmp(cc_lib[i], ccc)) {
                        ccc_ok = true;
                        break;
                    };
                }
                if (not ccc_ok)
                    logger.log_die("\nUnsupported CC class (-C option): %s. Use -h for help\n",ccc);
            }
            break;
        case 'U':
            parse_udt_option(optarg);
            break;
        case '?':
            logger.log_die("Unknown option: %s. Use -h for help\n", argv[optind-1]);
        default:
            break;
        }
    }
    
    if (setpgid(getpid(),getpid()) <0 ) {
	perror("setgrp failed");
	exit(1);
    }
    

    logger.setDebugLevel(debug_level);


    if ((3 > argc-optind))
        logger.log_die("missed arguments\n%s", usage);

    logger.setInteractive(false);

    fclose(stdin);
    
    strncpy(sSocks_port, argv[optind], MAX_NAME_SZ);
    strncpy(sPeer_listen_port, argv[optind+1], MAX_NAME_SZ);

    char fmt[256] = "%";
    sprintf(&fmt[strlen(fmt)],"%d",MAX_NAME_SZ);
    strcat(fmt,"[^:]:%");
    sprintf(&fmt[strlen(fmt)],"%d",MAX_NAME_SZ);
    strcat(fmt,"s");

    sscanf(argv[optind+2],fmt, sPeer_addr, sPeer_remote_port);

    if (!sPeer_remote_port[0])
        strncpy(sPeer_remote_port, sPeer_listen_port, MAX_NAME_SZ);
    
    //addrinfo *pServaddr;

    logger.log_debug(1, "peer addr = %s peer_rport = %s peer_lport = %s\n",
              sPeer_addr, sPeer_remote_port, sPeer_listen_port);
    
    if (demonize) { 
       switch (fork()) {
       case 0:  /* Child (left running on it's own) */
          break;
       case -1:
          perror("fork failed");
          exit(1);
       default:  /* Parent */
          exit(0);
       }
       setsid();  /* Move into a new session */
       fclose(stdout);
       fclose(stderr);
    }

    logger.log_info("");
    logger.log_info("Starting.\n");
    
    do { // just block
    	int pid = 0;
    	int server_pid = 0;
    	int client_pid = 0;
    	
    	
    	if (mode == DUAL or mode == SERVER) { // udt->tcp
    		server_pid = fork(); 
			if (server_pid==-1) logger.log_die("% fork failed.\n", peer_ident);
			if (!server_pid) {
				mode = SERVER;
				serv_ident = peer_ident;
				break;
			}
    	}
    	if (mode == DUAL or mode == CLIENT) { // tcp->udt
    		client_pid = fork();
			if (client_pid==-1) logger.log_die("% fork failed.\n", sock_ident);
			if (!client_pid) {
				mode = CLIENT;
				serv_ident = sock_ident;
				break;
			}
    	}
 	
    	signal(SIGTERM, exit_handler);
    	signal(SIGINT, exit_handler);
    	signal(SIGQUIT, exit_handler);
    	
    	do {
    		setpgid(getpid(),getpid());
    		if((pid = wait(NULL)) > 0) break;
    	}
    	while (errno == EINTR);
    	
    	if (pid == server_pid)
    		logger.log_notice("%s died. exiting.\n", peer_ident);
    	if (pid == client_pid)
    		logger.log_notice("%s died. exiting.\n", sock_ident);
    	
    	exit_handler(SIGCHLD);

    } while (false);
    
	utl::initsetproctitle(argc, argv, envp);
	utl::setproctitle("%s: %s", app_ident, serv_ident);

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char * listen_port = mode == SERVER ? sPeer_listen_port : sSocks_port;

    if (0 != getaddrinfo(NULL, listen_port, &hints, &pServaddr))
    {
        logger.log_die("Illegal port number or port is busy.\n");
        return 1;
    }


    if (0 != getaddrinfo(sPeer_addr, sPeer_remote_port, &hints, &pPeeraddr))
    {
        logger.log_die("Incorrect peer network address.\n");
        return 1;
    }
    
    if (rendezvous) {
        if (0 != getaddrinfo(NULL, sPeer_rzv_lport, &hints, &pPeerRzvLocal))
        {
            logger.log_die("Incorrect peer network address.\n");
            return 1;
        }
        if (0 != getaddrinfo(sPeer_addr, sPeer_rzv_rport, &hints, &pPeerRzvRemote))
        {
            logger.log_die("Incorrect peer network address.\n");
            return 1;
        }
    }

    
    if (mode == SERVER) {
        if(!rendezvous) {
            ludtsock = UDT::socket(pServaddr->ai_family, pServaddr->ai_socktype, pServaddr->ai_protocol);
            setsockopt(ludtsock);
            if (UDT::ERROR == UDT::bind(ludtsock, pServaddr->ai_addr, pServaddr->ai_addrlen))
            {
                logger.log_err("udt bind: %s\n", UDT::getlasterror().getErrorMessage());
                return 0;
            }
    
            if (UDT::ERROR == UDT::listen(ludtsock, 10))
            {
                logger.log_err("udt listen: %s\n", UDT::getlasterror().getErrorMessage());
                return 0;
            }
            logger.log_info("%s is ready at port: %s\n", peer_ident, sPeer_listen_port);
        }
    }
    else {
        int on = 1;
        ltcpsock = socket(pServaddr->ai_family, pServaddr->ai_socktype, pServaddr->ai_protocol);
        if (setsockopt(ltcpsock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on))
            logger.log_die("setsockopt() error.");
        if (-1 == bind(ltcpsock, pServaddr->ai_addr, pServaddr->ai_addrlen))
        {
            logger.log_err("tcp bind: %s\n",strerror(errno));
            return 0;
        }
        if (listen(ltcpsock, 10))
        {
            logger.log_err("tcp listen error\n");
            return 0;
        }
        logger.log_info("%s is ready at port: %s\n", sock_ident, sSocks_port);
    }
    //cout << "server is ready at port: " << service << endl;

    sockaddr_storage clientaddr;
    int addrlen = sizeof(clientaddr);


    while (true)
    {
        UDTSOCKET audtsock;
        int atcpsock;
        pthread_t childthread;
        char clienthost[NI_MAXHOST];
        char clientservice[NI_MAXSERV];


        if (mode == SERVER) {
            if (!rendezvous) {
                if (UDT::INVALID_SOCK == (audtsock = UDT::accept(ludtsock, (sockaddr*)&clientaddr, &addrlen)))
                {
                    logger.log_err("accept: %s\n", UDT::getlasterror().getErrorMessage());
                    return 0;
                }
            } else {
                audtsock = UDT::socket(pServaddr->ai_family, pServaddr->ai_socktype, pServaddr->ai_protocol);
                
                if (UDT::ERROR == UDT::bind(audtsock, pServaddr->ai_addr, pServaddr->ai_addrlen))
                {
                    logger.log_err("udt bind: %s\n", UDT::getlasterror().getErrorMessage());
                    return 0;
                }
                
                setsockopt(audtsock);
                
                int rc;
                while((rc = UDT::connect(audtsock, pPeerRzvRemote->ai_addr, pPeeraddr->ai_addrlen)) < 0) {
                    if (rc < 5000) {
                        //logger.log_notice("cannot connect to peer: %s\n", 
                        //        UDT::getlasterror().getErrorMessage());
                        //sleep(1);
                    }
                    else {
                        logger.log_err("peer connection fatal error: %s", 
                                UDT::getlasterror().getErrorMessage());
                        return 0;
                    }
                }
                int len = sizeof(sockaddr);
                UDT::getpeername(audtsock, (sockaddr *) &clientaddr, &len);
            }
            
            getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
            
            if (not utl::sockaddr_match((sockaddr*)(&clientaddr), pPeeraddr->ai_addr)) {
                    close(audtsock);
                    logger.log_notice("rejected peer connection: %s:%s\n", clienthost, clientservice);
                    continue;
            }
            if (debug_level or track_connections)
            	logger.log_notice("accepted peer connection: %s:%s\n", clienthost, clientservice);

            setsockopt(audtsock);
            pthread_create(&childthread, NULL, start_child,  &audtsock);
        }
        else {
            if (-1 == (atcpsock = accept(ltcpsock, (sockaddr*)&clientaddr, (socklen_t*) &addrlen)))
            {
                logger.log_err("accept: error: %s\n", strerror(errno));
                return 0;
            }
            getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
            
            if (net_access < 2) { // not -N -N options
            	bool subnet = net_access > 0 ? true: false; // at least -N option - subnet mode
            	if ( not utl::check_source(atcpsock, (sockaddr *)&clientaddr, subnet)) {
            		close(atcpsock);
            		logger.log_notice("rejected socks connection: %s:%s\n", clienthost, clientservice);
            		continue;
            	}
            }
            if (debug_level or track_connections)
            	logger.log_notice("accepted socks connection: %s:%s\n", clienthost, clientservice);
            pthread_create(&childthread, NULL, start_child,  &atcpsock);
        }
    }
    if (mode == SERVER)
        UDT::close(ludtsock);
    else
        close(ltcpsock);

    return 1;
}
void* start_child(void *servsock) {

    struct cargs_t cargs;
    pthread_t rcvthread, sndthread;
    struct sock_pkt spkt;
    char c;
    struct sockaddr clientaddr;
    int addrlen = sizeof(sockaddr);
    char clienthost[NI_MAXHOST];
    char clientservice[NI_MAXSERV];
    char * conntype;

    char buf[1024];

    cargs.shutdown = 0;

    if (mode == SERVER) {
        UDT::getpeername(*(UDTSOCKET*) servsock,&clientaddr, &addrlen);
        conntype = "UDT";
    }
    else {
        getpeername(*(int*) servsock, &clientaddr, (socklen_t*) &addrlen);
        conntype = "TCP";
    }
    getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
    
    
    while (true) {
        if (mode == SERVER) { // utd -> tcp
            cargs.udtsock = *(UDTSOCKET*) servsock;

            logger.log_debug(2, "udt accepted\n");
            
            // receive sizeof(spkt) bytes
            char * spktp = (char*) &spkt;
            for (char * p = spktp; p < spktp + sizeof(spkt);) {
            	p += UDT::recv(cargs.udtsock, p, spktp + sizeof(spkt) - p, 0);
            };

            logger.log_debug(2, "udt <- soks (-) recv\n");

            if(spkt.vn != 4) {
                logger.log_warning("wrong socks version (%d), connection closed\n", spkt.vn);
                spkt.cd = 91;
                UDT::send(cargs.udtsock, (char*) &spkt, sizeof(spkt), 0);
                return 0;
            }
            if(spkt.cd != 1) {
                logger.log_warning("! wrong command - only \"connect\" is supported\n");
                spkt.cd = 91;
                UDT::send(cargs.udtsock,(char*) &spkt, sizeof(spkt), 0);
                return 0;
            }
            
           
            do {
                UDT::recv(cargs.udtsock,&c, 1, 0);
            } while (c != 0);

            logger.log_debug(2, "udt <- soks (+) recv\n");

            int peersock = socket(AF_INET,SOCK_STREAM,0);

            sockaddr_in paddr;
            memset(&paddr,0,sizeof(paddr));

            paddr.sin_family = AF_INET;

            paddr.sin_port = *(short unsigned*) spkt.dstport;

            memcpy(&paddr.sin_addr, spkt.dstip, 4);

            if (connect(peersock, (sockaddr*) &paddr, sizeof(paddr)) == -1) {
                spkt.cd = 92;
                UDT::send(cargs.udtsock,(char*) &spkt, sizeof(spkt), 0);
                logger.log_warning("canot connect\n");
                break;
            }

            logger.log_debug(2, "connected\n");

            spkt.cd = 90;
            UDT::send(cargs.udtsock,(char*) &spkt, sizeof(spkt), 0);
            logger.log_debug(2, "udt <- soks send\n");
            cargs.tcpsock = peersock;
        }
        else { // tcp -> udt

            cargs.tcpsock = *(int*) servsock;

            recv(cargs.tcpsock,&spkt, sizeof(spkt), MSG_WAITALL);

            logger.log_debug(2, "tcp -> socks recv (-)\n");

            if(spkt.vn != 4) {
                logger.log_warning("wrong socks version; connection closed\n");
                spkt.cd = 91;
                send(cargs.tcpsock,&spkt, sizeof(spkt), 0);
                return 0;
            }
            if(spkt.cd != 1) {
                logger.log_warning("wrong command - only connect is supported\n");
                spkt.cd = 90;
                send(cargs.tcpsock,&spkt, sizeof(spkt), 0);
                return 0;
            }
            
            do {
                recv(cargs.tcpsock,&c, 1, 0);

            } while (c != 0);

            logger.log_debug(2, "tcp -> socks recv (+)\n");

            UDTSOCKET peersock = UDT::socket(pPeeraddr->ai_family, pPeeraddr->ai_socktype, pPeeraddr->ai_protocol);
            setsockopt(peersock);

            if (rendezvous) {
                if (UDT::ERROR == UDT::bind(peersock, pPeerRzvLocal->ai_addr, pPeerRzvLocal->ai_addrlen))
                {
                    logger.log_die("udt bind: %s\n", UDT::getlasterror().getErrorMessage());
                    return 0;
                }
            }
            
            logger.log_debug(1, "open UDT connection to: %s\n", utl::dump_inetaddr((sockaddr_in *) pPeeraddr->ai_addr, buf, true));

            if (UDT::ERROR == UDT::connect(peersock, pPeeraddr->ai_addr, pPeeraddr->ai_addrlen))
            {
                logger.log_warning("udt connect: %s\n", UDT::getlasterror().getErrorMessage());
                spkt.cd = 92;
                send(cargs.tcpsock,&spkt, sizeof(spkt), 0);
                break;
            }

            logger.log_debug(1, "udt connected\n");

            UDT::send(peersock,(char*) &spkt, sizeof(spkt), 0);
            UDT::send(peersock,"", sizeof(""), 0);

            logger.log_debug(2, "udt <- soks send\n");

            while(UDT::recv(peersock, (char*) &spkt, sizeof(spkt), 0) == 0);

            logger.log_debug(2, "udt -> soks recv\n");

            send(cargs.tcpsock,&spkt, sizeof(spkt), 0);

            logger.log_debug(2, "tcp <- soks send\n");

            if (spkt.cd != 90) {
                logger.log_err("remote peeer socks error\n");
                break;
            }

            cargs.udtsock = peersock;
        }
        pthread_create(&rcvthread, NULL, peer2sock_worker, &cargs);
        pthread_create(&sndthread, NULL, sock2peer_worker, &cargs);
        pthread_join(rcvthread, NULL);
        pthread_join(sndthread, NULL);
        break;
    }


    //log_debug("closing udt connection ...");
    UDT::close(cargs.udtsock);
    //logger.log_debug("... chosing udt connection : ok");
    //logger.log_debug("closing tcp connection ...");
    close(cargs.tcpsock);
    //logger.log_debug("... closing tcp connection : ok");
    //logger.log_debug("");
    if (debug_level or track_connections)
    	logger.log_notice("close %s connection: %s:%s\n", conntype, clienthost, clientservice);
    return NULL;
}

bool is_custom_value(UDTOpt optcode) {
    UDT_OPTIONS_T::iterator it = udt_options.find(optcode);
    return it == udt_options.end() ? false : true;
}

// Setup UDP buffer sizes.
// Returns "true" if system udp buffer size restrictions was applied.
bool check_udp_buffer(UDTSOCKET sock, UDTOpt optcode) {

    bool sysctl_ok = false;

    int unsigned size;
    int unsigned max_size = 0; // not limited
    int nlen, optsz;
    size_t    *szptr;

    assert(optcode == UDP_RCVBUF || optcode == UDP_SNDBUF);

    UDT::getsockopt(sock, 0, optcode, &size, &optsz);

/*
#ifdef 	OS_FREEBSD
    int var[] = {CTL_KERN, KERN_IPC, KIPC_MAXSOCKBUF}; // "kern.ipc.maxsockbuf";
    nlen = 3;
    sysctl_ok = true;
#endif
#ifdef	OS_LINUX
    int rvar[] = {CTL_NET, NET_CORE, NET_CORE_RMEM_MAX}; // "net.core.rmem_max";
    int wvar[] = {CTL_NET, NET_CORE, NET_CORE_WMEM_MAX}; // "net.core.wmem_max";
    int * var;
    var = optcode == UDP_RCVBUF ? rvar : wvar;
    nlen = 3;
    sysctl_ok = true;
#endif

    if (sysctl_ok)
        sysctl(var, nlen, &max_size, szptr, NULL, 0);
*/

#ifdef 	OS_FREEBSD
    max_size = 64*1024;
#endif
    
    if (max_size == 0)
        return true; 
    
    char* die_format = "requested custom %s value is too big. Maximal possible size = %d\n";
    char* warn_format = "default %s = %d is reduced to the maximal possible system size = %d\n";

    if (is_custom_value(optcode)) {
        if (max_size and size > max_size) {
            logger.log_die(die_format, optcode == UDP_RCVBUF ? "UDP_RCVBUF" : "UDP_SNDBUF" , max_size);
        }
        else
            UDT::setsockopt(sock, 0, optcode, &udt_options[optcode], sizeof(int));
    }
    else
        if (size > max_size) {
            UDT::setsockopt(sock, 0, optcode, &max_size, sizeof(int));
            logger.log_warning(warn_format, optcode == UDP_RCVBUF ? "UDP_RCVBUF" : "UDP_SNDBUF", size, max_size);
        }
}

void setsockopt(UDTSOCKET sock) {

    // set custom UDT options:
    for (UDT_OPTIONS_T::iterator it = udt_options.begin(); it != udt_options.end(); it++)
        UDT::setsockopt(sock, 0,  it->first, &it->second, sizeof(int));

    // work around system buffer size restrictions (typical for FreeBSD)
    check_udp_buffer(sock, UDP_RCVBUF);
    check_udp_buffer(sock, UDP_SNDBUF);

    /*
     UDT::setsockopt(sock, 0, UDT_RCVBUF, new int(rcvbuffer*2), sizeof(int));
     UDT::setsockopt(sock, 0, UDT_SNDBUF, new int(rcvbuffer*2), sizeof(int));
     */

    //// 
    // set RCVTIMEO = 100 ms
    //
    UDT::setsockopt(sock, 0, UDT_RCVTIMEO, new int(100), sizeof(int));

    ////
    // setup rendezvous mode if -R flag
    //
    if(rendezvous)
        UDT::setsockopt(sock, 0, UDT_RENDEZVOUS, new bool(true), sizeof(bool));

    // set UDP port range to bind if the option UDP_BASEPORT exists (???)
#ifdef UDP_BASEPORT_OPTION
    if (baseport) UDT::setsockopt(*sock, 0, UDP_BASEPORT, new int(baseport), sizeof(int));
    if (naxport)  UDT::setsockopt(*sock, 0, UDP_POOLSIZE, new int(maxport-baseport), sizeof(int));
#endif

    // {"Vegas","TCP","ScalableTCP","HSTCP","BiCTCP", "Westwood", "FAST"};

    if (! strcasecmp(ccc, "")) {
        //UDT::setsockopt(sock, 0, UDT_CC, new CCCFactory<CUDTCC>, sizeof(CCCFactory<CUDTCC>));
    }
    else if (! strcasecmp(ccc, "UDT")) {
        UDT::setsockopt(sock, 0, UDT_CC, new CCCFactory<CUDTCC>, sizeof(CCCFactory<CUDTCC>));
    }
    else if (! strcasecmp(ccc, "Vegas")) {
        UDT::setsockopt(sock, 0, UDT_CC, new CCCFactory<CVegas>, sizeof(CCCFactory<CVegas>));
    }
    else if (! strcasecmp(ccc, "TCP")) {
        UDT::setsockopt(sock, 0, UDT_CC, new CCCFactory<CTCP>, sizeof(CCCFactory<CTCP>));
    }
    else if (! strcasecmp(ccc, "ScalableTCP")) {
        UDT::setsockopt(sock, 0, UDT_CC, new CCCFactory<CScalableTCP>, sizeof(CCCFactory<CScalableTCP>));
    }
    else if (! strcasecmp(ccc, "HSTCP")) {
        UDT::setsockopt(sock, 0, UDT_CC, new CCCFactory<CHSTCP>, sizeof(CCCFactory<CHSTCP>));
    }
    else if (! strcasecmp(ccc, "BiCTCP")) {
        UDT::setsockopt(sock, 0, UDT_CC, new CCCFactory<CBiCTCP>, sizeof(CCCFactory<CBiCTCP>));
    }
    else if (! strcasecmp(ccc, "Westwood")) {
        UDT::setsockopt(sock, 0, UDT_CC, new CCCFactory<CWestwood>, sizeof(CCCFactory<CWestwood>));
    }
    else if (! strcasecmp(ccc, "FAST")) {
        UDT::setsockopt(sock, 0, UDT_CC, new CCCFactory<CFAST>, sizeof(CCCFactory<CFAST>));
    }
    else {
        cerr << "\nunsupprted congetion control class (option -c): " << ccc<< endl;
        exit(-1);
    }
}
void* sock2peer_worker(void * ar )
{
    cargs_t * pCargs = (cargs_t *) ar;
    char* data;
    data = new char[rcvbuffer];

    struct pollfd pfd[1];

    pfd[0].fd = pCargs->tcpsock;
    pfd[0].events = POLLIN;

    logger.log_debug(2, "start tcp->udt thread\n");

    while (true) {

        int sz, pollr;

        pollr = poll(pfd,1,10);
        if (pollr == 0)
            if (pCargs->shutdown == 1)
                break;
            else
                continue;

        sz = recv(pCargs->tcpsock, data, rcvbuffer, 0);

        if (sz == -1) {
            logger.log_notice("socks client recv error\n");
            break;
        }
        if (sz == 0) {
            break;
        }
        logger.log_debug(3, "tcp recv %d bytes\n", sz);
        if (UDT::send(pCargs->udtsock, data, sz, 0) == UDT::ERROR) {
            logger.log_err("udt send error: %s\n", UDT::getlasterror().getErrorMessage());
            break;
        }
        logger.log_debug(3, "udt send %d bytes\n", sz);
    }

    delete [] data;

    pCargs->shutdown = 1;

    //close(cargsp->tcpsock);
    //UDT::close(cargsp->udtsock);

    logger.log_debug(2, "stop tcp->udt thread\n");
    return NULL;
}
void* peer2sock_worker(void* ar)
{
    cargs_t * pCargs = (cargs_t *) ar;
    char* data;
    data = new char[rcvbuffer];

    logger.log_debug(2, "start udt->tcp thread\n");

    while (true)
    {
        int sz;
        do {
            sz =  UDT::recv(pCargs->udtsock, data, rcvbuffer, 0);
            //cout << "###" << endl;
            if (pCargs->shutdown) break;
        } while (sz == 0);
        logger.log_debug(3,"udt recv %d bytes\n", sz);
        if (pCargs->shutdown)
            break;
        if (UDT::ERROR == sz)
        {
            if (UDT::getlasterror().getErrorCode() == 2001)
                logger.log_notice("udt peer connection closed\n");
            else
                logger.log_err("udt recv: %s", UDT::getlasterror().getErrorMessage());

            pCargs->shutdown = 0;
            break;
        }
        if (send (pCargs->tcpsock, data, sz, 0) == -1 ) {
            cerr << "send error" << endl;
            break;
        }
        logger.log_debug(3,"tcp send %d bytes\n", sz);
    }

    delete [] data;
    pCargs->shutdown = 1;

    //close(cargsp->tcpsock);
    //UDT::close(cargsp->udtsock);

    logger.log_debug(2, "stop udt->tcp thread\n");
    return NULL;
}
int parse_udt_option (char * optstr) {
    char * optname = (char*) malloc(strlen(optstr));
    int    optval;

    sscanf(optstr, "%[a-zA-Z_]=%d", optname, &optval);
    {
        char q = optstr[strlen(optstr)-1];
        if      (q=='k' or q=='K')
            optval *= 1024;
        else if (q=='m'or q=='M')
            optval *= 1024*1024;
    }
    for(int i = 0; i < sizeof(UDT_OPTIONS_LIST)/sizeof(*UDT_OPTIONS_LIST); i++) {
        if(!strcasecmp(UDT_OPTIONS_LIST[i].optname, optname)) {
            udt_options[UDT_OPTIONS_LIST[i].optcode] = optval;
            return 0;
        };
    }
    logger.log_die("\nUnknown UDT option: %s\n", optname);
    exit(1);
}
void exit_handler(int sig) {
    logger.log_debug("got signal %d\n", sig);
	kill(0,SIGTERM);
	while(wait(NULL) != -1);
    logger.log_die("");
}
