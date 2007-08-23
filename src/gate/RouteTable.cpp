#include "common.h"
#include <nptl/pthread.h>
#include "RouteTable.h"

RouteTable::RouteTable()
{
    mutex = Mutex::init();
};

std::list<Route> RouteTable::getRoutes() {
    return routes;
}

Route* RouteTable::lookup(uint32_t ip) {
    Route * r = NULL;
    Mutex m(&mutex);

    for(std::list<Route>::iterator it = routes.begin(); it != routes.end(); it++) {
        if (it->match_ip(ip)) {
            if (r == NULL) {
                r = &(*it);
            }
            else if (it->masklen() > r->masklen()) {
                r = &(*it);
            }
        }
    }
    return r;
}

int RouteTable::pack(char* data, int max_size) {

    route_rec * ptr = (route_rec *) data;
    int sz = 0;

    for(std::list<Route>::iterator it = routes.begin();  it !=routes.end(); it++) {
        if((sz += sizeof(route_rec)) > max_size)
            log_msg(LOG_ERR, LOG_DIE, "route table too long.");
        it->pack(ptr++);
    }
    return sz;
}

void RouteTable::clear() {
    Mutex m(&mutex);
    routes.clear();
}

void RouteTable::add(Route& r) {
    Mutex m(&mutex);
    routes.push_back(r);
}
