
#include "adx_type.h"
#include "adx_alloc.h"
#include "adx_event.h"
#include "adx_network.h"
#include "adx_command_parse.h"

#define STATUS_UNDONE      1
#define STATUS_SUCCESS     2
#define STATUS_ERROR       3
#define STATUS_RECV_FULL   4
#define STATUS_SEND_FULL   5

typedef struct {

    int fd;
    int status;

    char *buf;
    int len, total;

    command_t cmd;

} redis_client_t;

#define IO_SIZE 1024
redis_client_t *client_create(int size)
{
    redis_client_t *client = je_malloc(sizeof(redis_client_t) * size);

    int i;
    for (i = 0; i < size; i++) {
        client[i].total = IO_SIZE;
        client[i].buf = je_malloc(IO_SIZE);
    }

    return client;
}

void demo_send(adx_event_t *event_loop, int fd, void *arg)
{
#if 0
    redis_client_t *c = arg;
    fprintf(stdout, "demo_send :: fd=%d\n", fd);

    // int size = write(c->fd, c->send_buf + c->send_success_len, c->send_len - c->send_success_len);
    //if (size >= c->send_len - c->send_success_len) {

    int size = write(c->fd, c->send_buf, c->send_len);
    if (size >= c->send_len) {
        adx_event_mod(event_loop, fd, demo_recv, NULL, c);
        return;
    }

    if (size < 0) {
        fprintf(stdout, "write :: close\n");
        return;
    }

    if (size == 0) {
        fprintf(stdout, "write :: close\n");
        close(fd);
        return;
    }

    // c->send_success_len += size;
#endif
}

void demo_recv(adx_event_t *event_loop, int fd, void *arg)
{
    redis_client_t *c = arg;
    // fprintf(stdout, "demo_recv :: fd=%d\n", fd);

    int size = read(fd, c->buf + c->len, c->total - c->len);
    if (size == -1) {
        if (errno == EINTR || errno == EAGAIN)
            return;

        close(fd);
        fprintf(stdout, "read : err\n");
        return;
    }

    if (size == 0) {
        close(fd);
        fprintf(stdout, "read : close\n");
        return;
    }

    c->len += size;
    c->buf[c->len] = 0;

    c->cmd.buf = c->buf;
    c->cmd.len = c->len;

    // fprintf(stdout, "%s\n", c->buf);
    command_parse(&c->cmd);

    if (c->cmd.status == COMMAND_SUCCESS || c->cmd.status == COMMAND_ERROR) {

        char *str = "$2\r\nOK\r\n";
        write(c->fd, str, strlen(str));

        command_free(&c->cmd);
        c->len = 0;
    }
}

void demo_accept(adx_event_t *event_loop, int sock_listen, void *arg)
{
    int fd = adx_network_accept(sock_listen, NULL, NULL);
    if (fd <= 0) return;

    // fprintf(stdout, "demo_accept :: fd=%d\n", fd);

    redis_client_t *array = (redis_client_t *)arg;
    redis_client_t *c = &array[fd];

    c->fd = fd;
    c->len = 0;
    c->cmd.status = COMMAND_UNDONE;

    adx_event_add(event_loop, fd, demo_recv, NULL, c);
}

int main()
{
    adx_event_t *event_loop = adx_event_create();
    fprintf(stdout, "epfd=%d\n", event_loop->epfd);

    int sock_listen = adx_network_listen(6379);
    fprintf(stdout, "sock_listen=%d\n", sock_listen);

    redis_client_t *c = client_create(1024);
    adx_event_add(event_loop, sock_listen, demo_accept, NULL, c);
    adx_event_wait(event_loop);
    return 0;
}





