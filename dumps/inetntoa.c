#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

int main()
{
    struct in_addr addr1, addr2;
    char str1[30], str2[30];

    addr1.s_addr = htonl(0x1020304);
    addr2.s_addr = htonl(0xc0a80101);

    char *buf;
    buf = inet_ntoa(addr1);
    strcpy(str1, buf);
    buf = inet_ntoa(addr2);
    strcpy(str2, buf);

    printf("%#lx -> %s \n", (long)addr1.s_addr, str1);
    printf("%#lx -> %s \n", (long)addr2.s_addr, str2);

    return 0;
}