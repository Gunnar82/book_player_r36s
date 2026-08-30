#ifndef LOCAL_DELETE_H
#define LOCAL_DELETE_H

#include "storage.h"

int local_delete_file(const char *path, const StoragePath roots[], int root_count);
int local_delete_directory(const char *path, const StoragePath roots[], int root_count);

#endif
