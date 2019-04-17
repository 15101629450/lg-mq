
#include "adx_event.h"

#define DEFAULT_EVENT_SIZE 1024

adx_event_t *adx_event_create()
{
    adx_event_t *event_loop = je_malloc(sizeof(adx_event_t));
    if (!event_loop) return NULL;

    event_loop->epfd = epoll_create(DEFAULT_EVENT_SIZE);
    if (event_loop->epfd == -1) goto err;

    event_loop->ep_list = je_malloc(sizeof(struct epoll_event) * DEFAULT_EVENT_SIZE);
    if (!event_loop->ep_list) goto err;

    event_loop->info_list = je_malloc(sizeof(adx_event_info_t) * DEFAULT_EVENT_SIZE);
    if (!event_loop->info_list) goto err;

    event_loop->size = DEFAULT_EVENT_SIZE;
    return event_loop;
err:

    if (event_loop->ep_list) je_free(event_loop->ep_list);
    if (event_loop->info_list) je_free(event_loop->info_list);
    je_free(event_loop);
    return NULL;
}

void adx_event_free(adx_event_t *event_loop)
{
    if (!event_loop) return;
    if (event_loop->ep_list) je_free(event_loop->ep_list);
    if (event_loop->info_list) je_free(event_loop->info_list);
    je_free(event_loop);
}

static adx_event_info_t *get_adx_event_info(adx_event_t *event_loop, int fd)
{
    if (fd < 0 || fd >= event_loop->size)
        return NULL;
    return &event_loop->info_list[fd];
}

static int adx_event_ctl(adx_event_t *event_loop, int fd, adx_event_call_t *r, adx_event_call_t *w, void *arg, int status)
{
    struct epoll_event e = {0};
    if (!get_adx_event_info(event_loop, fd))
        return -1;

    e.data.fd = fd;
    e.events |= r ? EPOLLIN : 0;
    e.events |= w ? EPOLLOUT : 0;

    event_loop->info_list[fd].r = r;
    event_loop->info_list[fd].w = w;
    event_loop->info_list[fd].arg = arg;

    return epoll_ctl(event_loop->epfd, status, fd, &e);
}

int adx_event_add(adx_event_t *event_loop, int fd, adx_event_call_t *r, adx_event_call_t *w, void *arg)
{
    return adx_event_ctl(event_loop, fd, r, w, arg, EPOLL_CTL_ADD);
}

int adx_event_mod(adx_event_t *event_loop, int fd, adx_event_call_t *r, adx_event_call_t *w, void *arg)
{
    return adx_event_ctl(event_loop, fd, r, w, arg, EPOLL_CTL_MOD);
}

int adx_event_del(adx_event_t *event_loop, int fd, adx_event_call_t *r, adx_event_call_t *w, void *arg)
{
    return 0;
}

void adx_event_wait_loop(adx_event_t *event_loop)
{
    int n, fds = epoll_wait(event_loop->epfd, event_loop->ep_list, event_loop->size, -1);
    if (fds <= 0) return;

    for (n = 0; n < fds; n++) {

        struct epoll_event *e = &event_loop->ep_list[n];
        adx_event_info_t *info = get_adx_event_info(event_loop, e->data.fd);
        if (info) {

            if (e->events & EPOLLIN && info->r) info->r(event_loop, e->data.fd, info->arg);
            if (e->events & EPOLLOUT && info->w) info->w(event_loop, e->data.fd, info->arg);
        }
    }
}

void adx_event_wait(adx_event_t *event_loop)
{
    for (;;) {
        
        adx_event_wait_loop(event_loop);
    }
}




