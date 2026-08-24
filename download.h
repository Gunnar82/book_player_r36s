#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <stddef.h>

#define REMOTE_MAX_ENTRIES 512
#define REMOTE_NAME_LEN 512
#define REMOTE_MTIME_LEN 64
#define REMOTE_PATH_LEN 2048

typedef enum {
    REMOTE_DIRECTORY = 1,
    REMOTE_FILE = 2
} RemoteEntryType;

typedef struct {
    RemoteEntryType type;
    char name[REMOTE_NAME_LEN];
    long long size;
    char mtime[REMOTE_MTIME_LEN];
} RemoteEntry;

typedef struct {
    RemoteEntryType type;
    char relative_path[REMOTE_PATH_LEN];
} RemoteSelection;

typedef int (*DownloadProgressFn)(const char *folder,
                                  const char *name,
                                  int file_index,
                                  int file_count,
                                  long long file_now,
                                  long long file_total,
                                  long long total_now,
                                  long long total_size,
                                  void *userdata);

int remote_fetch_listing(const char *relative_path,
                         RemoteEntry *entries,
                         int max_entries,
                         char *error,
                         size_t error_size);

int remote_download_files(const char *relative_path,
                          const RemoteEntry *entries,
                          int entry_count,
                          DownloadProgressFn progress,
                          void *userdata,
                          char *error,
                          size_t error_size);

/* Laedt ausgewaehlte Dateien oder Verzeichnisse. Verzeichnisse werden
 * rekursiv traversiert; lokal bleibt der Pfad relativ zu base_url erhalten. */
int remote_download_selection(const RemoteSelection *selection,
                              int selection_count,
                              DownloadProgressFn progress,
                              void *userdata,
                              char *error,
                              size_t error_size);

#endif
