
#ifndef __ADX_NETWORK_H__
#define	__ADX_NETWORK_H__

#ifdef __cplusplus
extern "C" { 
#endif

    int adx_network_check(int fd);
    int adx_network_not_wait(int fd);
    int adx_network_not_block(int fd);
    int adx_network_reuse_addr(int fd); // sock 重用
    int adx_network_fork_bind(int fd); // 多进程绑定
    int adx_network_send_timeout(int fd, int usec);
    int adx_network_recv_timeout(int fd, int usec);
    int adx_network_set_kernel_buffer(int fd, int send_size, int recv_size);
    int adx_network_get_kernel_buffer(int fd, int *send_size, int *recv_size);
    int adx_network_set_keepalive(int fd);
    char *adx_network_host_to_ip(char *host, char *ip_addr);

    int adx_network_listen(int port);
    int adx_network_accept(int sock_listen, struct sockaddr *addr, socklen_t *addrlen);
    int adx_network_connect(char *addr, int port);

#ifdef __cplusplus
}
#endif

#endif


