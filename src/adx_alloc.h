
#ifndef __ADX_ALLOC_H__
#define __ADX_ALLOC_H__

#include "adx_type.h"
#include "adx_list.h"

#ifdef __cplusplus
extern "C" { 
#endif

    typedef struct {
        adx_list_t head;
    } adx_pool_t;

    adx_pool_t *adx_pool_create();
    void adx_pool_free(adx_pool_t *pool);

    void *adx_alloc(adx_pool_t *pool, size_t size);

#ifdef __cplusplus
}
#endif

#endif


