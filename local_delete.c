#include "local_delete.h"

#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int path_inside_root(const char *path, const char *root, int allow_root)
{
    char resolved_path[PATH_MAX];
    char resolved_root[PATH_MAX];

    if (!path || !root || !realpath(path, resolved_path) || !realpath(root, resolved_root)) {
        return 0;
    }

    size_t root_len = strlen(resolved_root);
    while (root_len > 1 && resolved_root[root_len - 1] == '/') {
        resolved_root[--root_len] = '\0';
    }

    if (!strcmp(resolved_path, resolved_root)) {
        return allow_root;
    }

    return !strncmp(resolved_path, resolved_root, root_len) && resolved_path[root_len] == '/';
}

static int path_allowed(const char *path, const StoragePath roots[], int root_count, int allow_root)
{
    if (!path || !path[0] || !roots || root_count <= 0) {
        return 0;
    }

    for (int i = 0; i < root_count; i++) {
        if (roots[i].path[0] && path_inside_root(path, roots[i].path, allow_root)) {
            return 1;
        }
    }

    return 0;
}

static int remove_tree(const char *path)
{
    struct stat st;
    if (lstat(path, &st) != 0) {
        return -1;
    }

    if (!S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode)) {
        return unlink(path);
    }

    DIR *dir = opendir(path);
    if (!dir) {
        return -1;
    }

    int result = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }

        char child[PATH_MAX];
        int written = snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(child) || remove_tree(child) != 0) {
            result = -1;
            break;
        }
    }

    closedir(dir);
    if (result != 0) {
        return -1;
    }

    return rmdir(path);
}

int local_delete_file(const char *path, const StoragePath roots[], int root_count)
{
    struct stat st;
    if (!path_allowed(path, roots, root_count, 0) || lstat(path, &st) != 0) {
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        return -1;
    }

    return unlink(path);
}

int local_delete_directory(const char *path, const StoragePath roots[], int root_count)
{
    struct stat st;
    if (!path_allowed(path, roots, root_count, 0) || lstat(path, &st) != 0) {
        return -1;
    }

    if (!S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode)) {
        return -1;
    }

    return remove_tree(path);
}
