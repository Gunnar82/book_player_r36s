#ifndef NETWORK_LIMITS_H
#define NETWORK_LIMITS_H

#include <stddef.h>
#include <stdlib.h>
#include <curl/curl.h>

/*
 * Radio Browser XML can contain thousands of stations. A parsed StreamEntry
 * is roughly 4.5 KiB because it keeps long URL and favicon fields, so the
 * dynamically grown station array can legitimately exceed 16 MiB.
 * Keep a hard ceiling, but leave enough room for ~10k station responses.
 */
#define NETWORK_MAX_DYNAMIC_ALLOCATION (64U * 1024U * 1024U)

static inline void *network_limited_realloc(void *ptr,size_t size)
{
    if(size>NETWORK_MAX_DYNAMIC_ALLOCATION)return NULL;
    return realloc(ptr,size);
}

static inline CURLcode network_hardened_curl_perform(CURL *curl)
{
    if(!curl)return CURLE_BAD_FUNCTION_ARGUMENT;
    curl_easy_setopt(curl,CURLOPT_PROTOCOLS_STR,"http,https");
    curl_easy_setopt(curl,CURLOPT_REDIR_PROTOCOLS_STR,"http,https");
    return curl_easy_perform(curl);
}

#define realloc(ptr,size) network_limited_realloc((ptr),(size))
#define curl_easy_perform(curl) network_hardened_curl_perform((curl))

#endif
