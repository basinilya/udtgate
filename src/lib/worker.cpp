#include <time.h>
#include <utils.h>
#include <logger.h>
#include <socket_api.h>

extern Logger logger;

int utl::worker (int fd_in, int fd_out, int sd, timeval * timeout) {

    int   bfsize = 256*1024;
    char * data = new char[bfsize];

    SOCK_API::FDSET rset;
    SOCK_API::FDSET wset;
    SOCK_API::FDSET eset;

    logger.log_debug(2, "start worker\n");

    char str[globals::dump_message*5+10];

    int maxfdn = SOCK_API::maxfdn(fd_out, sd);

    bool active       = true;
    bool fwd_shut     = false;
    bool rev_shut     = false;
    time_t start;
    time_t stamp;
    
    start = stamp = time(0);

    while (!fwd_shut and !rev_shut
            and (timeout == NULL or time(0) - stamp < timeout->tv_sec)) {

        timeval th, to;
        th.tv_sec = 0;
        th.tv_usec = 1000;

        rset.ZERO();
        wset.ZERO();
        eset.ZERO();
        
        rset.SET(fd_in);
        rset.SET(sd);

        eset.SET(fd_in);
        eset.SET(fd_out);
        eset.SET(sd);


        //logger.log_debug(3,"  Select ...\n");
        
        if (active) {
            to.tv_sec = 0;
            to.tv_usec = 0;
            //logger.log_debug(3," rd select (active)... \n");
            SOCK_API::select(maxfdn, &rset, NULL, &eset, NULL);
            //logger.log_debug(3," ... rd select (active)\n");
        }
        else {
            to = th;
            //logger.log_debug(3," rd select (pass)... \n");
            SOCK_API::select(maxfdn, &rset, NULL, &eset, &to);
            //logger.log_debug(3," ... rd select (pass)\n");
        }
        
        active = false;
        
        if (rset.ISSET(fd_in)) {
            wset.SET(sd);
        }
        if (rset.ISSET(sd)) {
            wset.SET(fd_out);
        }
        
        to.tv_sec = 0;
        to.tv_usec = 0;
        //logger.log_debug(3," wr select ... \n");
        SOCK_API::select(maxfdn, NULL, &wset, &eset, &to);
        //logger.log_debug(3," ... wr select \n");


        // fd_in -> sd
        if (rset.ISSET(fd_in) and wset.ISSET(sd) and !fwd_shut) {

            int sz1  = 0;
            active = true;

            //logger.log_debug(3," read ... \n");
            sz1 = SOCK_API::read(fd_in, data, bfsize);
            //logger.log_debug(3," ... read \n");

            
            while (1) {
                if (sz1 > 0) {
                    //logger.log_debug(3," writen ... \n");
                    int sz2 = SOCK_API::writen(sd, data, sz1);
                    //logger.log_debug(3," ... writen\n");
                    if (sz2 == sz1) {
                        logger.log_debug(3,"  sd1 -> sd2 %d/%d bytes: [%s]\n", sz1, sz2,
                                         utl::dump_str(str, data, bfsize, sz2, globals::dump_message));
                        stamp = time(0);
                        break;
                    }
                    else {
                        logger.log_debug(3,"  sd1 -> sd2 close(write error)\n");
                        fwd_shut= true;
                    }
                }
                else if (sz1 == 0) {
                    logger.log_debug(3,"  sd1 -> sd2 EOF\n");
                    fwd_shut= true;
                    // nothing
                }
                else {
                    logger.log_debug(3,"  sd1 -> sd2 close(read error)\n");
                    fwd_shut = true;
                }
                SOCK_API::shutdown(fd_in, SHUT_RD);
                SOCK_API::shutdown(sd, SHUT_WR);
                break;
            }
        }

        // sd -> fd_out
        if (rset.ISSET(sd) and wset.ISSET(fd_out) and !rev_shut) {

            int sz1 = 0;
            active = true;

            sz1 = SOCK_API::read(sd, data, bfsize);
            //logger.log_debug(3," worker sd2 read %d bytes from %d\n", sz1,sd);
            while(1) {
                if (sz1 > 0) {
                    int sz2 = SOCK_API::writen(fd_out, data, sz1);
                    if (sz2 == sz1) {
                        logger.log_debug(3,"  sd2 -> sd1 %d/%d bytes: [%s]\n", sz1, sz2,
                                         utl::dump_str(str, data, bfsize, sz2, globals::dump_message));
                        stamp = time(0);
                        break;
                    }
                    else {
                        logger.log_debug(3,"  sd2 -> sd1 close (write error)\n");
                        rev_shut = true;
                    }
                }
                else if (sz1 == 0) {
                    logger.log_debug(3,"  sd1 -> sd2 EOF\n");
                    rev_shut= true;
                    // nothing
                }
                else {
                    logger.log_debug(3,"  sd2 -> sd1 close(read error)\n");
                    rev_shut = true;
                }
                SOCK_API::shutdown(sd, SHUT_RD);
                SOCK_API::shutdown(fd_out, SHUT_WR);
                break;
            }
        }
    }
    SOCK_API::close(fd_in);
    if (fd_in != fd_out)
        SOCK_API::close(fd_out);
    SOCK_API::close(sd);

    delete [] data;

    logger.log_debug(2, "stop worker\n");
    return 0;
}

int utl::worker (int sd1, int sd2, timeval * to) {
    return utl::worker (sd1, sd1, sd2);
}
