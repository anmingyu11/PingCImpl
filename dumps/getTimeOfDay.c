#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

// 计算时间差，单位：ms
float timediff(
    struct timeval *begin,
    struct timeval *end)
{
    int n;
    n = (end->tv_sec - begin->tv_sec) * 1000000 + (end->tv_usec - begin->tv_usec); //一秒等于百万分之一微秒

    return (float)(n / 1000);
}

int main()
{
    struct timeval begin, end;

    gettimeofday(&begin, 0);

    printf("wait a second....\n");

    sleep(1);

    gettimeofday(&end, 0);

    printf("running time : %fms\n", timediff(&begin, &end));

    return 0;
}