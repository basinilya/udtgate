#define  DEFAULT_INCLUDES
#include <utils.h>
#include <logger.h>

Logger logger("test: ");

int main(int argc, char* argv[])
{
    int fd;
    sockaddr_in addr;
    char * a;
    bool subnet = false;
    char buf[256];

    if (argc > 1)
        a = argv[1];
    else
        a = "127.0.0.1";

    if (argc > 2)
        subnet = true;

    addr.sin_family = AF_INET;

    if(inet_aton(a,&addr.sin_addr) == 0)
        perror("aton");

    utl::dump_inetaddr(&addr, buf);
    printf("1: addr = %s\n", buf);


    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket:");
        exit(1);
    }

    if (utl::check_source((sockaddr*) &addr, subnet))
        printf("Matched\n");
    else
        printf("Not matched\n");
}
