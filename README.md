# Ping程序的C语言实现

###1. 使用方法
```SHELL
gcc ping.c -o ping
sudo ./ping ${hostIp}
```
----
###2. 原理
----
####2.1 ping命令原理详解
ping 命令是用来查看网络上另一个主机系统的网络连接是否正常的一个工具

由上面命令的执行结果可以看到，ping 命令执行后依次显示出

- 被测试系统主机名和相应 IP 地址
- 返回给当前主机的 ICMP 报文顺序号
- ttl 生存时间和往返时间 rtt（单位是毫秒，即千分之一秒）。

要真正了解 ping 命令实现原理，就要了解 ping 命令所使用到的 TCP/IP 协议：[ICMP 协议](http://blog.csdn.net/snlying/article/details/4211396) .

ICMP 是（Internet Control Message Protocol）Internet 控制报文协议。它是 TCP/IP 协议族的一个子协议，用于在 IP 主机、路由器之间传递控制消息。

ICMP是IP协议不可分割的一部分,ICMP是属于网络层的一个协议.

控制消息有

- 目的不可达消息
- 超时信息
- 重定向消息
- 时间戳请求和时间戳响应消息
- 回显请求和回显应答消息。

ping 命令的机制是回显请求和回显应答消息。具体表现是向网络上的另一个主机系统发送 ICMP 报文，如果指定系统得到了报文，它将把报文一模一样地传回给发送者。

回显请求和回显应答消息 ICMP 报文格式:

|类型|代码|校验和|
|------|------|------|
|标识符|序号|
|数据|

类型字段:
回显请求报文其中类型为 0，代码为 0。
回显应答报文其中类型为 8，代码为 0。

校验和字段：
包括数据在内的整个 ICMP 协议数据包的校验和，具体实现方法会在下面详细介绍。

标识符字段：
用于唯一标识 ICMP 报文，本项目使用程序的进程 id。因为如果同时在两个命令行终端执行 ping 命令的话，每个 ping 命令都会接收到所有的回显应答，所以需要根据标识符来判断回显应答是否应该接收。

序号字段：
ICMP 报文的序号。

数据字段：

也就是报文，本项目中我们将发送报文的时间戳放入数据字段，这样当接收到该报文应答的时候可以取出发送时间戳，将接收应答的时间戳减去发送时间戳就是报文往返时间（rtt）。提前预告一下，这里使用gettimeofday()API函数获取时间戳，详细介绍会在函数介绍部分说明。
```C
//ICMP 报文 C 语言实现可以用下面的数据结构表示：
struct icmp{
        unsigned char   type;         // 类型
        unsigned char   code;         // 代码
        unsigned short  checksum;     // 校验和
        unsigned short  id;            // 标识符
        unsigned short  sequence;     // 序号
        struct timeval  timestamp;    // 时间戳 
};
```
unsigned char 正好一字节，也就是 8bit，unsigned short 二字节，也就是 16bit，unsigned int4 字节（32bit），不过上面没用到。struct timeval 后面说明.

----
####IP报文格式
系统发送ICMP报文时会将ICMP报文作为IP的数据，也就是放入IP报文格式的数据字段，IP报文格式如下图所示：

|4位版本|4位首部长度|8位服务类型|16位总长度|
|-----|-----|-----|-----|
|16位标识|3位标识|13位片偏移|
|8位生存时间(TTL)|8位协议|16位首部校验和|
|32位源ip地址|
|32位目的ip地址|
|数据|

注意上面4位首部长度和16位总长度，4位首部长度不包括数据字段，并且单位是4字节。16位总长度包括数据字段，单位是1字节。

IP报文首部C语言实现可以用下面的数据结构表示：

```
struct ip{
    unsigned char version:4;       // 版本
    unsigned char hlen:4;        // 首部长度
    unsigned char tos;             // 服务类型
    unsigned short len;         // 总长度
    unsigned short id;            // 标识符
    unsigned short offset;        // 标志和片偏移
    unsigned char ttl;            // 生存时间
    unsigned char protocol;     // 协议
    unsigned short checksum;    // 校验和
    struct in_addr ipsrc;        // 32位源ip地址
    struct in_addr ipdst;       // 32位目的ip地址
};

```
注意ICMP协议是IP层的一个协议，这里有人可能不清楚为什么ICMP协议是IP层的。

这是因为ICMP报文在发送给报文接收方时可能要经过若干子网，会牵涉到路由选择等问题，所以ICMP报文需通过IP协议来发送，是IP层的。

####2.2地址信息表示

当我们编写网络应用程序时，必然要使用地址信息指定数据传输给网络上哪个主机，那么地址信息应该包含哪些呢？

1.地址族，基于IPv4的地址族还是IPv6的地址族。
2.IP地址。
3.端口号。

为了便于记录地址信息，系统定义了如下结构体：

```
struct sockaddr_in{
    sa_family_t        sin_family;     // 地址族
    uint16_t        sin_port;        // 端口号
    struct in_addr  sin_addr;          // 32位IP地址
    char             sin_zero[8];    // 不使用
};
```

其中struct in_addr结构体定义如下：

```
struct in_addr{
    in_addr_t     s_addr;        // 32位IP地址
};
```
in_addr_t使用如下宏指令定义，也就是无符号整型32位。

```
#define in_addr_t uint32_t
```

但实际上，还有一种结构体也可以表示地址信息，如下所示：
```
struct sockaddr{
    sa_family_t    sin_family;     // 地址族
    char         sa_data[14]; // IP地址和端口
};
```

成员sa_data保存的信息包含 IP 地址和端口号，剩余部分填充0。

在网络编程中，常用的是 struct sockaddr_in 结构体，因为相对于 struct sockaddr 结构体，前者填充数据比较方便。

不过网络编程接口函数定义使用的是 struct sockaddr 结构体类型，这是由于最先使用的是 struct sockaddr 结构体，struct sockaddr_in 结构体是后来为了方便填充地址信息数据定义。这就出现矛盾了，不过也不用担心上面两个结构体之间是可以相互转换的。定义地址信息时使用 struct sockaddr_in 结构体，然后将该结构体类型转为 struct sockaddr 结构体类型传递给网络编程接口函数即可。

####2.3 主机字节序与网络字节序

4字节整型数值1可用二进制表示如下：

00000000 00000000 00000000 00000001

而有些CPU则以倒序保存。

00000001 00000000 00000000 00000000

所以，如果发送主机与接收主机CPU字节序不一样则就会出现问题。

引申上面的问题，这就涉及到CPU解析数据的方式，其方式有2种：

大端序（Big Endian）：高位字节存放到低位地址。

小端序（Little Endian）：高位字节存放到高位地址。由于不同CPU字节序不一样，因此，在通过网络传输数据时约定统一方式，这种约定称为网络字节序（Network Byte Order），非常简单——统一为大端序。

所以，进行网络传输数据前，需要先把数据数组转化为大端序格式再进行网络传输。接收到网络数据后，需要转换本机字节序格式然后进行后续处理。不过，幸运地是这些工作不需要我们自己完成，系统会自动转换的。

我们唯一需要转换的是向struct sockaddr_in结构体变量填充IP地址和端口号数据的时候。当然，系统已经提供了一些函数，只需调用相应函数即可。

转换字节序的函数有：

```
unsigned short htons(unsigned short);
unsigned short ntohs(unsigned short);
unsigned long  htonl(unsigned long);
unsigned long  ntohl(unsigned long);
```

上面的函数非常简单，通过函数名就能知道它们的功能，htonl/htons 中的h代表主机（host）字节序，n代表网络（network）字节序，s指的是 short，l指的是 long（需要注意一下，linux 中 long 类型只占4个字节，跟int类型一样）。

实例:
```C
#include <stdio.h>
#include <arpa/inet.h>

int main(void)
{
	unsigned short hosts = 0x1234;
	unsigned short nets;
	unsigned long hostl = 0x12345678;
	unsigned long netl;

	nets = htons(hosts);
	netl = htonl(hostl);

    printf("Host ordered short: %#x \n", hosts);
    printf("Network ordered short: %#x \n", nets);

    printf("Host ordered long: %#lx \n", hostl);
    printf("Network ordered long: %#lx \n", netl);
}
```
----
####2.4 函数介绍

介绍几个重要的函数及其作用

- gettimeofday() 精确存入当前时间
- inet_addr() 转换点分十进制-->32位整型
- inet_ntoa() 转换32位整形-->点分十进制
- gethostbyname() 通过域名得到IP
----
####函数gettimeofday()。

```
#include <sys/time.h>
int gettimeofday(struct timeval *tv, struct timezone *tz);
```

该函数的作用是把当前的时间放入 struct timeval 结构体中返回。

注意：
1.精确级别,微秒级别
2.受系统时间修改影响
3.返回的秒数是从1970年1月1日0时0分0秒开始

其参数 tv 是保存获取时间结果的结构体，参数 tz 用于保存时区结果。

结构体 timeval 的定义为：

```
struct timeval
{
    long int tv_sec;      // 秒数
    long int tv_usec;     // 微秒数
}
```

结构体timezone的定义为：

```
struct timezone
{
    int tz_minuteswest;/*格林威治时间往西方的时差*/
    int tz_dsttime;    /*DST 时间的修正方式*/
}
```
timezone 参数若不使用则传入0即可，本项目传入0。

实例:
```C
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

// 计算时间差，单位：ms
float timediff(
    struct timeval *begin,
    struct timeval *end
){
    int n;
    n = (end->tv_sec - begin->tv_sec) * 1000000 + (end->tv_usec - begin->tv_usec);//一秒等于百万分之一微秒

    return (float)(n/1000);
}

int main(){
    struct timeval begin,end;

    gettimeofday(&begin,0);

    printf("wait a second....\n");

    sleep(1);

    gettimeofday(&end,0);

    printf("running time : %fms\n", timediff(&begin, &end));

    return 0;
}
```
----

####函数 inet_addr 
```C
#include <arpa/inet.h>
in_addr_t inet_addr(const char *string);
```
该函数的作用是将用点分十进制字符串格式表示的IP地址转换成32位大端序整型。

成功时返回32位大端序整型数值，失败时返回 INADDR_NONE 。

实例:
```
#include <stdio.h>
#include <arpa/inet.h>

int main()
{
    char *addr1 = "1.2.3.4";
    char *addr2 = "192.168.1.1";

    in_addr_t data;

    data = inet_addr(addr1);
    printf(" %s -> %#lx \n", addr1, (long)data);

    data = inet_addr(addr2);
    printf(" %s -> %#lx \n", addr2, (long)data);

    return 0;
}
```
----
####函数inet_ntoa
```C
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
```

inet_addr函数在执行过程中，在内部会申请内存并保存结果字符串，然后返回内存地址。所以调用完该函数应该将字符串复制到其他内存空间。因为再次调用该函数时，之前保存的字符串很有可能被覆盖。

来源: 实验楼
链接: https://www.shiyanlou.com/courses/305
本课程内容，由作者授权实验楼发布，未经允许，禁止转载、下载及非法传播
来源: 实验楼
链接: https://www.shiyanlou.com/courses/305
本课程内容，由作者授权实验楼发布，未经允许，禁止转载、下载及非法传播