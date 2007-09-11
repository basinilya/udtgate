#include <utils.h>
#include <logger.h>
#include <socket_api.h>

extern Logger logger;

int utl::worker (int fd_in, int fd_out, int sd, timeval * to) {

    int   bfsize = 16*1024;
    char * data = new char[bfsize];

    SOCK_API::FDSET rset;
    SOCK_API::FDSET wset;
    SOCK_API::FDSET eset;

    logger.log_debug(2, "start worker\n");

    bool fd_in_to_sd  = true;
    bool sd_to_fd_out = true;

    char str[globals::dump_message*5+10];

    int maxfdn = SOCK_API::maxfdn(fd_out, sd);

    bool runing = false;

    while (fd_in_to_sd or sd_to_fd_out) {

        timeval th, *to;
        th.tv_sec = 3;
        th.tv_usec = 0;

        rset.ZERO();
        wset.ZERO();
        eset.ZERO();
        
        rset.SET(fd_in);
        rset.SET(sd);

        wset.SET(fd_out);
        wset.SET(sd);

        eset.SET(fd_in);
        eset.SET(fd_out);
        eset.SET(sd);

        to = runing ? NULL : &th;

        //logger.log_debug(3,"  Select ...\n");
        SOCK_API::select(maxfdn, &rset, &wset, &eset, to);
        //logger.log_debug(3,"  ... select\n");

        runing = false;

        // fd_in -> sd
        if (fd_in_to_sd and
            rset.ISSET(fd_in) and
            wset.ISSET(sd)) {

            int sz1  = 0;
            int sz2  = 0;

            runing = true;

            sz1 = SOCK_API::read(fd_in, data, bfsize);
            if (sz1 > 0) {
                sz2 = SOCK_API::writen(sd, data, sz1);
            }
            if (sz2 > 0) {
                logger.log_debug(3,"  sd1 -> sd2 %d/%d bytes: [%s]\n", sz1, sz2,
                                 utl::dump_str(str, data, bfsize, sz2, globals::dump_message));
            }
            if (sz2 <= 0) {
                if (sz2 < 0)
                    logger.log_debug(3,"  sd1 -> sd2 (close/error)\n");
                else
                    logger.log_debug(3,"  sd1 -> sd2 (close/normal)\n");
                fd_in_to_sd = false;
            }
        }

        // sd -> fd_out
        if (sd_to_fd_out and
            rset.ISSET(sd) and
            wset.ISSET(fd_out)) {

            int sz1 = 0;
            int sz2 = 0;

            runing = true;

            sz1 = SOCK_API::read(sd, data, bfsize);
            logger.log_debug(3," worker sd2 read %d bytes from %d\n", sz1,sd);

            if (sz1 > 0) {
                sz2 = SOCK_API::write(fd_out, data, sz1);
            }
            if (sz2 > 0)
                logger.log_debug(3,"  sd2 -> sd1 %d/%d bytes: [%s]\n", sz1, sz2,
                                 utl::dump_str(str, data, bfsize, sz2, globals::dump_message));
            if (sz2 <= 0) {
                if (sz2 < 0)
                    logger.log_debug(3,"  sd2 -> sd1 (close/error)\n");
                else
                    logger.log_debug(3,"  sd2 -> sd1 (close/normal)\n");
                sd_to_fd_out = false;
            }
        }
    }

    delete [] data;

    logger.log_debug(2, "stop worker\n");
    return 0;
}

int utl::worker (int sd1, int sd2, timeval * to) {
    return utl::worker (sd1, sd1, sd2);
}
