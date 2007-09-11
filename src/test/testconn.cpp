#include <sys/wait.h>
#include <signal.h>
#include <udt/udt.h>
#include <udt/cc.h>
#include <setproctitle.h>
#define  DEFAULT_INCLUDES
#include <udtgate.h>
#include <utils.h>
#include <stdlib.h>
#include <logger.h>

using namespace std;

char addr[256];
char port[256];
char socksaddr[256] = "localhost";
char socksport[256] = "1080";

int sd;
bool noproxy = false;

addrinfo hints, *paddr, *psocksaddr;
 
char usage[] = "usage:\n"
        "        ugw_testconn addr:port [socksaddr:[socksport]]\n"
        "        ugw_testconn addr:port direct\n";

Logger logger("testconn");

int main(int argc, char* argv[], char* envp[])
{
    if (argc < 2) {
        cerr << "ugw_testconn - simple sock4 connection cleint.\n\n" << usage << endl;
        exit(0);
    }
    if (sscanf(argv[1],"%[^:]:%[^:]",&addr, &port) !=2) {
        cerr << "wrong arguments\n\n" << usage << endl;
        exit(1);
    }

    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (0 != getaddrinfo(addr, port, &hints, &paddr)) {
        cerr << "can not resolv address: " << addr << ":" << port << endl;
        exit(1);
    }
    
    if (argc > 2)
        if (0 == strcasecmp(argv[2], "noproxy")) {
            noproxy = true;
        }
        else if (sscanf(argv[2],"%[^:]:%[^:]",&socksaddr, &socksport) < 1) {
            cerr << "wrong arguments\n\n" << usage << endl;
            exit(1);
        }
    
    if (0 != getaddrinfo(socksaddr, socksport, &hints, &psocksaddr)) {
        cerr << "can not resolv address: " << socksaddr << ":" << socksport << endl;
    }
    
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 1) {
        perror("socket: ");
        exit(1);
    }
    
    logger.setDebugLevel(3);
    
    if (noproxy) {
        if (connect(sd, paddr->ai_addr, paddr->ai_addrlen) != 0) {
            perror("connect: ");
            exit(1);
        }
    }
    else {
        if (connect(sd, psocksaddr->ai_addr, psocksaddr->ai_addrlen) != 0) {
            perror("connect: ");
            exit(1);
        }
        
        sock_pkt req;
        
        req.vn = 4;
        req.cd = 1;
        memcpy(&req.dstport,&((sockaddr_in*) paddr->ai_addr)->sin_port, 
                sizeof(req.dstport));
        memcpy(&req.dstip,&((sockaddr_in*) paddr->ai_addr)->sin_addr, 
                sizeof(req.dstip));

        send(sd, &req, sizeof(req),0);
        char null = '\0';
        send(sd, &null, 1,0);
        
        //fflush(sd);
        recv(sd, &req, sizeof(req),0);
    }

    cout << "success!!!\n";
    
    utl::worker (fileno(stdin),fileno(stdout),sd);
}
