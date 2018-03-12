#include <stdio.h>   
#include <stdlib.h>   
#include <netdb.h>   
#include <sys/socket.h>   
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    struct hostent *host;

    if (argc < 2)
    {
        printf("Use : %s <hostname> \n", argv[0]);
        exit(1);
    }

    host = gethostbyname(argv[1]);

    if(host == NULL){
        return 1;
    }
    for (int i = 0; host->h_addr_list[i]; ++i)
    {
        printf("IP addr %d : %s \n", i + 1,
               inet_ntoa(*(struct in_addr *)host->h_addr_list[i]));
    }
}
