#ifndef BACKGROUND_DOWNLOAD_H
#define BACKGROUND_DOWNLOAD_H

#include "download.h"
#include <stddef.h>

#define BACKGROUND_DOWNLOAD_MAX_SELECTIONS 256

typedef struct {
    int active;
    int finished;
    int cancelled;
    int result;
    int file_index;
    int file_count;
    long long file_now;
    long long file_total;
    long long total_now;
    long long total_size;
    double rate_bps;
    char folder[REMOTE_PATH_LEN];
    char filename[REMOTE_NAME_LEN];
    char error[256];
} BackgroundDownloadStatus;

int background_download_start(const RemoteSelection *selection,
                              int selection_count,
                              char *error,
                              size_t error_size);
void background_download_get_status(BackgroundDownloadStatus *status);
void background_download_cancel(void);
void background_download_wait(void);
void background_download_shutdown(void);

#endif
