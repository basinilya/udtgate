#include <utils.h>
#include <logger.h>
#include <socket_api.h>

extern Logger logger;

int utl::worker (int sd1, int sd2) {

    int   bfsize = 16*1024;
    char * data = new char[bfsize];

    SOCK_API::FDSET rset;
    SOCK_API::FDSET wset;
    SOCK_API::FDSET eset;

    logger.log_debug(2, "start worker\n");

    bool sd1_to_sd2  = true;
    bool sd2_to_sd1  = true;

    char str[globals::dump_message*5+10];

    int maxfdn = std::maxfdn(sd1, sd2);

    bool runing = false;

    while (sd1_to_sd2 or sd2_to_sd1) {

        timeval th, *to;
        th.tv_sec = 0;
        th.tv_usec = 1000;

        rset.ZERO();
        wset.ZERO();
        eset.ZERO();

        to = runing ? NULL : &th;

        SOCK_API::select(maxfdn, &rset, &wset, &eset, to);

        runing = false;

        // sd1 -> sd2
        if (sd1_to_sd2 and
            rset.ISSET(sd1) and
            wset.ISSET(sd2)) {

            int sz1  = 0;
            int sz2  = 0;

            runing = true;

            sz1 = SOCK_API::readn(sd1, data, bfsize, 0);
            if (sz1 > 0) {
                sz2 = SOCK_API::writen(sd2, data, sz1,0);
            }
            if (sz2 > 0) {
                logger.log_debug(3,"  tcp->udt %d/%d bytes: [%s]\n", sz1, sz2,
                                 utl::dump_str(str, data, bfsize, sz2, globals::dump_message));
            }
            if (sz2 <= 0) {
                logger.log_debug(2,"  tcp->udt (close)\n");
                sd1_to_sd2 = false;
            }
        }

        // udt->tcp
        if (sd2_to_sd1 and
            rset.ISSET(sd2) and
            wset.ISSET(sd1)) {

            int sz1 = 0;
            int sz2 = 0;

            runing = true;

            sz1 = SOCK_API::readn(sd2, data, bfsize);

            if (sz1 > 0) {
                sz2 = SOCK_API::writen(sd1, data, sz1);
            }
            if (sz2 > 0)
                logger.log_debug(3,"  tcp->udt %d/%d bytes: [%s]\n", sz1, sz2,
                                 utl::dump_str(str, data, bfsize, sz2, globals::dump_message));
            if (sz2 <= 0) {
                logger.log_debug(2,"  udt->tcp (close)\n");
                sd2_to_sd1 = false;
            }
        }
    }

    delete [] data;
    //close(cargsp->tcpsock);
    //UDT::close(cargsp->udtsock);

    logger.log_debug(2, "stop worker\n");
    return 0;
}
