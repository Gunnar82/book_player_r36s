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
        snprintf(path, sizeof(path), "%s/%s", dirpath, e->d_name);
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
        snprintf(path, sizeof(path), "%s/%s", base, e->d_name);
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

    char child_names[MAX_BOOKS][256];
    char child_paths[MAX_BOOKS][512];
    int child_count = scan_books(dirpath, child_names, child_paths);
    for (int i = 0; i < child_count && *count < MAX_BOOKS; i++)
        scan_recursive_impl(child_paths[i], names, paths, count);
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
        snprintf(tracks[count].path, sizeof(tracks[count].path), "%s/%s", dirpath, e->d_name);
        count++;
    }
    closedir(dir);
    qsort(tracks, count, sizeof(Track), cmp_track);
    return count;
}
