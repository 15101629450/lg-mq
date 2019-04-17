
#ifndef __ADX_QUEUE_H__
#define __ADX_QUEUE_H__

#ifdef __cplusplus
extern "C" { 
#endif

#include "adx_type.h"
#include "adx_list.h"

    /**********/ 
    /* STRING */
    /**********/ 
    typedef struct {
        char *str;
        int len;
    } adx_str_t;

    adx_str_t adx_str_init(const char *s, int len); 
    adx_str_t adx_strdup(adx_str_t str);
    adx_str_t adx_strdup_str(const char *s, int len);

    int adx_empty(adx_str_t str);
    void adx_str_free(adx_str_t str);

    /*********/ 
    /* QUEUE */
    /*********/ 
    typedef struct {
        adx_list_t queue;
        adx_str_t str;
    } adx_queue_t;

    adx_str_t adx_queue_push(adx_list_t *queue, adx_str_t str);
    adx_str_t adx_queue_pop(adx_list_t *queue);
    void adx_queue_clear(adx_list_t *queue);

    adx_str_t adx_queue_push_dup(adx_list_t *queue, adx_str_t str);
    adx_str_t adx_queue_push_str(adx_list_t *queue, const char *buf, int len);

#define adx_queue_init adx_list_init

#ifdef __cplusplus
}
#endif

#endif

