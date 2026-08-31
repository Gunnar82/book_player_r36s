#ifndef UPDATE_CHECK_H
#define UPDATE_CHECK_H

#define UPDATE_CHECK_VERSION_LEN 64
#define UPDATE_CHECK_URL_LEN 1024
#define UPDATE_CHECK_SHA256_LEN 65
#define UPDATE_CHECK_STATUS_LEN 160

typedef struct {
    char version[UPDATE_CHECK_VERSION_LEN];
    char binary_url[UPDATE_CHECK_URL_LEN];
    char sha256[UPDATE_CHECK_SHA256_LEN];
} UpdateManifest;

int update_check_latest(UpdateManifest *manifest, char *status, int status_size);
int update_version_compare(const char *a, const char *b);

#endif
