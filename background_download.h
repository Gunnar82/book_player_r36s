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

/* Startet genau einen Download-Job. Gibt 0 bei Erfolg, -1 bei Fehler oder
 * bereits laufendem Job zurueck. Die Auswahl wird intern kopiert. */
int background_download_start(const RemoteSelection *selection,
                              int selection_count,
                              char *error,
                              size_t error_size);

/* Liefert einen threadsicheren Snapshot des aktuellen Jobs. */
void background_download_get_status(BackgroundDownloadStatus *status);

/* Fordert einen laufenden Download zum kontrollierten Abbruch auf. */
void background_download_cancel(void);

/* Beim Programmende aufrufen. Wartet auf einen eventuell laufenden Worker. */
void background_download_shutdown(void);

#endif
