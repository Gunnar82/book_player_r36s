#ifndef NETWORK_LIMITS_H
#define NETWORK_LIMITS_H

#include <stddef.h>
#include <stdlib.h>

#define NETWORK_MAX_DYNAMIC_ALLOCATION (16U * 1024U * 1024U)

static inline void *network_limited_realloc(void *ptr,size_t size)
{
    if(size>NETWORK_MAX_DYNAMIC_ALLOCATION)return NULL;
    return realloc(ptr,size);
}

#define realloc(ptr,size) network_limited_realloc((ptr),(size))

#endif
