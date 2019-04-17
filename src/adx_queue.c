
#include <string.h>
#include "adx_queue.h"

/**********/ 
/* STRING */
/**********/ 
adx_str_t adx_str_init(const char *s, int len) 
{
    adx_str_t str = {(char *)s, len};
    return str;
}

int adx_empty(adx_str_t str)
{
    return (!str.str || !str.len) ? 1 : 0;
}

adx_str_t adx_strdup(adx_str_t str)
{
    adx_str_t dest = {0};
    if (adx_empty(str)) return dest;

    dest.str = je_malloc(str.len + 1);
    if (!dest.str) return dest;

    dest.len = str.len;
    dest.str[dest.len] = 0;
    memcpy(dest.str, str.str, str.len);
    return dest;
}

adx_str_t adx_strdup_str(const char *s, int len)
{
    adx_str_t str = adx_str_init(s, len);
    return adx_strdup(str);
}

void adx_str_free(adx_str_t str)
{
    if (!adx_empty(str)) je_free(str.str);
}

/*********/ 
/* QUEUE */
/*********/ 
adx_str_t adx_queue_push(adx_list_t *queue, adx_str_t str)
{
    if (adx_empty(str)) return adx_str_init(NULL, 0);

    adx_queue_t *node = je_malloc(sizeof(adx_queue_t));
    if (!node) return adx_str_init(NULL, 0);

    node->str = str;
    adx_list_add(queue, &node->queue);
    return node->str;
}

adx_str_t adx_queue_pop(adx_list_t *queue)
{
    adx_str_t str = {0};
    adx_queue_t *node = (adx_queue_t *)adx_queue(queue);
    if (node) {
        str = node->str;
        je_free(node);
    }

    return str;
}

adx_str_t adx_queue_push_dup(adx_list_t *queue, adx_str_t str)
{
    return adx_queue_push(queue, adx_strdup(str));
}

adx_str_t adx_queue_push_str(adx_list_t *queue, const char *buf, int len)
{
    adx_str_t str = adx_str_init(buf, len);
    return adx_queue_push_dup(queue, str);
}

void adx_queue_clear(adx_list_t *queue)
{
    for(;;) {
        adx_str_t str = adx_queue_pop(queue);
        if (adx_empty(str)) return;
        adx_str_free(str);
    }
}



