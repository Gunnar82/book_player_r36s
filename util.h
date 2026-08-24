#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

void util_trim(char *s);
int util_copy_checked(char *dst,size_t dst_size,const char *src);
int util_join_path_checked(char *dst,size_t dst_size,const char *a,const char *b);

#endif
