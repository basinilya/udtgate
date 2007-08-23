#include "udtgate.h"
#include "RouteTable.h"
#include "Peer.h"

using namespace std;

void read_config (char * file);
void parse_line  (string line);

void setsockopt(UDTSOCKET*);

RouteTable local_routes;
RouteTable routes;

addrinfo hints;


int daemon_opt = 0;
int default_ttl = 0;

int rcvbuffer = BLOCK_SIZE;

extern char *optarg;
extern int  optind, opterr;
char * config_file = "/etc/udtgate.conf";

peers_map peers;

int main(int argc, char* argv[]) {

    int c;
    static char optstring [] = "Dc:";
    opterr=0;
    UDTSOCKET ludtsock;
    int       ltcpsock = 0;

    char *usage = \
            "usage:"
            "  udtgate    [-D] -c <config.file>\n"
            "\t-D runs as daemon"
            "\t-c specify configuration file"
            "\n";

    while ((c=getopt(argc,argv,optstring)) != -1) {
        switch(c)
        {
            case 'D':
                daemon_opt = 1;
                break;
            case 'c':
                config_file = optarg;
                break;
            case '?':
                log_msg(LOG_ERR, LOG_DIE, "unknow option");
        }
    }

    try {
        read_config(config_file);
    }
    catch (ifstream::failure e) {
        log_msg(LOG_ERR, LOG_DIE, "can not read configuration file: %s\n", config_file);
    }
    catch (string msg) {
        log_msg(LOG_ERR, LOG_DIE, "config parse error:\n%s",  msg.c_str());
    }
    catch(exception& e) {
        cerr << e.what() << endl;
        log_msg(LOG_ERR, LOG_DIE, "???\n");
    }

    for(peers_map::iterator it = peers.begin(); it != peers.end(); it++) {
        log_tagmsg(it->second, LOG_MSG, LOG_IGN, "init\n");
        it->second->Init();
    }
    sleep(600);
}
void read_config (char * file) {

    string line;
    ifstream fin;

    fin.exceptions(ios::failbit | ios::badbit);
    fin.open(file);

    for (int n=1; fin.peek() != EOF;n++) {
        getline(fin,line);
        //__DEBUG__(line);
        if (line.length() == 0 || line.at(0) == '#' || line.at(0) == ' ' || line.at(0) == '!' || line.at(0) == '\n')
            continue;
        //__DEBUG__( "line:" << line);
        try {
            //__DEBUG__("try parseline");
            parse_line(line);
        }
        catch (string msg) {
            throw(string("") + "\nline: " + itostr(n) + "\n>>" + line + "<< - " + msg + "\n");
        }
        catch (exception& e) {
            __DEBUG__(e.what());
        }
        catch (...) {
            __DEBUG__("???");
        }
    }
}

void parse_line(string line) {

    string command;
    stringstream lines(line);

    lines >> command;
    if (command == kw::NETWORK) {
        string prefix; lines >> prefix;
        local_routes.add(*(new Route(prefix)));
    }
    else if (command == kw::BASE_PORT)   {
        Peer::setDefaultPort(resolvService(getString(lines)));
    }
    else if (command == kw::NEIGHBOR)  {
        Peer * peer;
        struct in_addr ip;
        string addr, subcomm;

        lines >> addr;

        if (!inet_aton(addr.c_str(),&ip))
            throw(string("bad ip address: ")+ addr);
        peers_map::iterator it = peers.find(ip);
        lines >> subcomm;
        if (subcomm == kw::PEER) {
            if (it == peers.end()) {
                string state;
                peers[ip] = peer = new Peer(ip);
                lines >> state;
                if (state == kw::UP)
                    peer->setShutdown(false);
                else if (state == kw::DOWN )
                    peer->setShutdown(true);
                else
                    throw(string("peer command requres <up> or <down> argument"));
            }
            else
                throw(string("only one <neighbor d.d.d.d peer> command is allowed"));
        }
        else {
            if (it != peers.end()) {
                peer = it->second;

                if (subcomm == kw::PEER_PORT) {
                    int peer_port;
                    lines >> peer_port; peer->setPeerPort(peer_port);
                }
                else if (subcomm == kw::LOCAL_ADDR) {
                    string  local_addr;
                    lines >> local_addr; peer->setLocalAddr(local_addr);
                }
                else if (subcomm == kw::TTL) {
                    int ttl;
                    lines >> ttl; peer->setTTL(ttl);
                }
                else
                    throw(string("unknown neighbor command: ")+subcomm);
            }
            else
                throw(string("<neighbor d.d.d.d peer> command should be first"));
        }
    }
    else
        throw(string("unknown command:")+command);
}
