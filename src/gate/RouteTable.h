#ifndef __ROUTETABLE_H
#define __ROUTETABLE_H	1

#include "udtgate.h"
#include "Route.h"
#include <list>

class RouteTable {
public:
    RouteTable();
    void add(Route&);
    void remove(Route);
    Route* lookup(uint32_t ip);
    int pack(char* data, int max_size);
    void clear();
    std::list<Route> getRoutes();
    pthread_mutex_t  mutex;

private:
    std::list<Route> routes;
};
#endif
