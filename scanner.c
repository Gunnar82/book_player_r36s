#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "scanner.h"
#include "config.h"

int is_audio(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (!dot) return 0;
    return !strcasecmp(dot, ".mp3") ||
           !strcasecmp(dot, ".ogg") ||
           !strcasecmp(dot, ".wav") ||
           !strcasecmp(dot, ".flac");
}

static int is_real_directory(const char *path)
{
    struct stat st;
    if (lstat(path, &st) != 0) return 0;
    /* Symlinks bewusst nicht verfolgen, damit keine Verzeichnisschleifen
       entstehen koennen. */
    return S_ISDIR(st.st_mode);
}

int directory_has_audio(const char *dirpath)
{
    DIR *dir = opendir(dirpath);
    if (!dir) return 0;

    int found = 0;
    struct dirent *e;
    while ((e = readdir(dir)) != NULL) {
        if (e->d_name[0] == '.') continue;
        if (is_audio(e->d_name)) { found = 1; break; }
    }
    closedir(dir);
    return found;
}

int directory_has_subdirectories(const char *dirpath)
{
    DIR *dir = opendir(dirpath);
    if (!dir) return 0;

    int found = 0;
    struct dirent *e;
    while ((e = readdir(dir)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char path[512];
        if (snprintf(path, sizeof(path), "%s/%s", dirpath, e->d_name) >= (int)sizeof(path))
            continue;
        if (is_real_directory(path)) { found = 1; break; }
    }
    closedir(dir);
    return found;
}

static int cmp_track(const void *a, const void *b)
{
    const Track *aa = a;
    const Track *bb = b;
    return strcasecmp(aa->name, bb->name);
}

int scan_books(const char *base, char names[][256], char paths[][512])
{
    DIR *dir = opendir(base);
    if (!dir) {
        fprintf(stderr, "Kann %s nicht oeffnen: %s\n", base, strerror(errno));
        return 0;
    }

    int count = 0;
    struct dirent *e;
    while ((e = readdir(dir)) && count < MAX_BOOKS) {
        if (e->d_name[0] == '.') continue;

        char path[512];
        if (snprintf(path, sizeof(path), "%s/%s", base, e->d_name) >= (int)sizeof(path))
            continue;
        if (!is_real_directory(path)) continue;

        snprintf(names[count], 256, "%s", e->d_name);
        snprintf(paths[count], 512, "%s", path);
        count++;
    }
    closedir(dir);

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (strcasecmp(names[i], names[j]) > 0) {
                char tmpn[256], tmpp[512];
                strcpy(tmpn, names[i]); strcpy(names[i], names[j]);
                strcpy(tmpp, paths[i]); strcpy(paths[i], paths[j]);
                strcpy(names[j], tmpn); strcpy(paths[j], tmpp);
            }
        }
    }
    return count;
}

static const char *path_basename(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static int cmp_string_ptr(const void *a, const void *b)
{
    const char *const *aa = a;
    const char *const *bb = b;
    return strcasecmp(*aa, *bb);
}

/*
 * Rekursiver Scan mit kleiner Stack-Nutzung: Unterverzeichnisse werden
 * dynamisch auf dem Heap gesammelt. Der alte Code legte pro Rekursionsebene
 * zwei MAX_BOOKS-grosse Arrays auf dem Stack an, was auf Handhelds bei tiefen
 * Verzeichnisbaeumen schnell mehrere hundert KiB pro Ebene verbrauchen konnte.
 */
static void scan_recursive_impl(const char *dirpath,
                                char names[][256], char paths[][512],
                                int *count)
{
    if (*count >= MAX_BOOKS) return;

    if (directory_has_audio(dirpath)) {
        snprintf(names[*count], 256, "%s", path_basename(dirpath));
        snprintf(paths[*count], 512, "%s", dirpath);
        (*count)++;
        if (*count >= MAX_BOOKS) return;
    }

    DIR *dir = opendir(dirpath);
    if (!dir) return;

    char **children = NULL;
    size_t child_count = 0;
    size_t child_cap = 0;
    struct dirent *e;

    while ((e = readdir(dir)) != NULL) {
        if (e->d_name[0] == '.') continue;

        char child[512];
        if (snprintf(child, sizeof(child), "%s/%s", dirpath, e->d_name) >= (int)sizeof(child))
            continue;
        if (!is_real_directory(child)) continue;

        if (child_count == child_cap) {
            size_t new_cap = child_cap ? child_cap * 2 : 8;
            char **grown = realloc(children, new_cap * sizeof(*children));
            if (!grown) break;
            children = grown;
            child_cap = new_cap;
        }

        children[child_count] = strdup(child);
        if (!children[child_count]) break;
        child_count++;
    }
    closedir(dir);

    qsort(children, child_count, sizeof(*children), cmp_string_ptr);
    for (size_t i = 0; i < child_count && *count < MAX_BOOKS; i++)
        scan_recursive_impl(children[i], names, paths, count);

    for (size_t i = 0; i < child_count; i++)
        free(children[i]);
    free(children);
}

int scan_books_recursive(const char *base, char names[][256], char paths[][512])
{
    int count = 0;
    if (!base || !*base) return 0;
    scan_recursive_impl(base, names, paths, &count);
    return count;
}

int scan_tracks(const char *dirpath, Track tracks[])
{
    DIR *dir = opendir(dirpath);
    if (!dir) return 0;

    int count = 0;
    struct dirent *e;
    while ((e = readdir(dir)) && count < MAX_TRACKS) {
        if (e->d_name[0] == '.' || !is_audio(e->d_name)) continue;
        snprintf(tracks[count].name, sizeof(tracks[count].name), "%s", e->d_name);
        if (snprintf(tracks[count].path, sizeof(tracks[count].path), "%s/%s", dirpath, e->d_name) >= (int)sizeof(tracks[count].path))
            continue;
        count++;
    }
    closedir(dir);
    qsort(tracks, count, sizeof(Track), cmp_track);
    return count;
}
