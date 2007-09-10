#include <udtgate.h>

int globals::net_access = 0;
int globals::debug_level = 0;
int globals::dump_message = 16;
bool globals::track_connections = false;
bool globals::rendezvous = false;
bool globals::demonize = false;

char * globals::app_ident = "?udtgate?";
char * globals::sock_ident =  "sock2peer server";
char * globals::peer_ident  = "peer2sock server";
char * globals::serv_ident  = "?udtgate?";
