
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "adx_network.h"

int adx_network_check(int fd)
{
    int error = -1;
    socklen_t len = sizeof(int);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
        return -1;
    return error;
}

int adx_network_not_wait(int fd)
{
    struct linger opt = {1, 0};
    return setsockopt(fd, SOL_SOCKET, SO_LINGER, &opt, sizeof(struct linger));
}

int adx_network_not_block(int fd)
{
    int opts = -1;
    if ((opts = fcntl(fd, F_GETFD, 0)) == -1)
        return -1;
    if ((fcntl(fd, F_SETFL, opts | O_NONBLOCK)) == -1)
        return -1;
    return 0;
}

// sockfd 重用
int adx_network_reuse_addr(int fd)
{
    int opts = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(int));
}

// 多进程绑定
int adx_network_fork_bind(int fd)
{
    int opts = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opts, sizeof(int));
}

int adx_network_send_timeout(int fd, int usec)
{
    struct timeval timeout = {0, usec};
    return setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
}

int adx_network_recv_timeout(int fd, int usec)
{
    struct timeval timeout = {0, usec};
    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
}

int adx_network_set_kernel_buffer(int fd, int send_size, int recv_size)
{
    int size = 0;
    if ((size = send_size))
        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(int)) == -1)
            return -1;

    if ((size = recv_size))
        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(int)) == -1)
            return -1;
    return 0;
}

int adx_network_get_kernel_buffer(int fd, int *send_size, int *recv_size)
{
    *send_size = *recv_size = -1;
    socklen_t len = sizeof(int);

    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, send_size, &len) == -1)
        return -1;

    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, recv_size, &len) == -1)
        return -1;

    return 0;
}

// #define TCP_KEEPALIVE 300
int adx_network_set_keepalive(int fd)
{
    int opts = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opts, sizeof(int)) == -1)
        return -1;
#ifdef TCP_KEEPALIVE
    /* 在Linux上默认设置为7200 */
    opts = TCP_KEEPALIVE;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &opts, sizeof(int)) < 0)
        return -1;

    opts = TCP_KEEPALIVE / 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &opts, sizeof(int)) < 0)
        return -1;

    /* 发送3个ACK 没有应答 */
    opts = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &opts, sizeof(int)) < 0)
        return -1;
#endif
    return 0;
}

char *adx_network_host_to_ip(char *host, char *ip_addr)
{
    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;  /* specify socktype to avoid dups */

    if ((getaddrinfo(host, NULL, &hints, &info)) != 0)
        return NULL;

    if (info->ai_family == AF_INET) {
        struct sockaddr_in *sa = (struct sockaddr_in *)info->ai_addr;
        inet_ntop(AF_INET, &(sa->sin_addr), ip_addr, 16);
    } else {
        struct sockaddr_in6 *sa = (struct sockaddr_in6 *)info->ai_addr;
        inet_ntop(AF_INET6, &(sa->sin6_addr), ip_addr, 32);
    }

    freeaddrinfo(info);
    return ip_addr;
}

int adx_network_listen(int port)
{
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s <= 0) return -1;

    if (adx_network_not_block(s) != 0) goto err;
    if (adx_network_reuse_addr(s) != 0) goto err;
    if (bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) goto err;
    if (listen(s, SOMAXCONN)) goto err;
    
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    return s;
err:    
    close(s);
    return -1;
}

int adx_network_accept(int sock_listen, struct sockaddr *addr, socklen_t *addrlen)
{
    for (;;) {
        int s = accept(sock_listen, addr, addrlen);
        if (s == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                return -1;
            }
        }

        if (adx_network_not_block(s) != 0) {
            close(s);
            return -1;
        }

        return s;
    }
}

int adx_network_recv(int fd, char *buf, int len)
{
    int size = read(fd, buf, len);
    if (size == 0) {
        close(fd);
        return -1;
    }

    if (size == -1) {
        if (errno == EINTR || errno == EAGAIN)
            return 0;

        return -1;
    }

    return size;
}



