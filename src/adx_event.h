
#ifndef __ADX_EVENT_H__
#define	__ADX_EVENT_H__

#include "adx_type.h"
#include "adx_list.h"
#include <sys/epoll.h>

#ifdef __cplusplus
extern "C" { 
#endif

    typedef struct adx_event_t adx_event_t;
    typedef struct adx_event_info_t adx_event_info_t;
    typedef void adx_event_call_t(adx_event_t *event_loop, int fd, void *arg);

    struct adx_event_t {

        int epfd;
        struct epoll_event *ep_list;
        adx_event_info_t *info_list;
        int size;
    };

    struct adx_event_info_t {

        adx_event_call_t *r;
        adx_event_call_t *w;
        void *arg;
    };

    adx_event_t *adx_event_create();
    void adx_event_free(adx_event_t *event_loop);

    int adx_event_add(adx_event_t *event_loop, int fd, adx_event_call_t *r, adx_event_call_t *w, void *arg);
    int adx_event_mod(adx_event_t *event_loop, int fd, adx_event_call_t *r, adx_event_call_t *w, void *arg);
    int adx_event_del(adx_event_t *event_loop, int fd, adx_event_call_t *r, adx_event_call_t *w, void *arg);

    void adx_event_wait(adx_event_t *event_loop);

#ifdef __cplusplus
}
#endif

#endif


