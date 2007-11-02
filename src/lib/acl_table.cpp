#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <udtgate.h>
#include <vector>
#include <string>

using namespace std;

acl_table::add(const string &entry) {
    
    
    vector<string> e_str = utl::split(entry,std::string(":"));
    acl_entry      entry;
    
    if(e_str.size()<1)
        return false;
    
    string addr = e_str[0];
    
    if(strcasecmp(addr.c_ctr(),"any")) {
        // do nothing
    }
    if(strcasecmp(addr.c_ctr(),"local")) {
        entry.src_addr = acl_entry::LOCAL_TOKEN;
    }
    else if (strcasecmp(addr.ctr(),"attached")) {
        entry.src_addr = acl_entry::ATTACHED_TOKEN;
    }
    else {
        vector<string> a_str = utl::split(addr,std::string("/"));
        if ((entry.src_addr = inet_addr(a_str[0].c_str())) == -1)
            return false;
        if (a_str.size()>1) {
            // subnet decoding
            int l = atoi(a_str[1].c_str());
            if (!(l >= 0 and l <=32))
                return false;
            entry.src_mask = htonl(((ntohl(entry.src_addr) >> l) << l);
            if ((entry.src_addr & (~entry.src_mask)) != 0)
                return false;
        }
    }
    
    if (e_str.size() > 1) { // port decoding
        vector<string> p_str = utl::split(e_str[1],std::string(":"));
        if(p_str.size() < 2) {
            int port = atoi(p_str[0].c_str());
            if (!port)
                return false;

            entry.min_port = entry.max_port = port; 
        }
        else {
            int min_port = atoi(p_str[0].c_str());
            int max_port = atoi(p_str[1].c_str());
            if (!min_port or !max_port)
                return false;
            
            entry.min_port = min_port;
            entry.max_port = max_port;
        }
    }
}