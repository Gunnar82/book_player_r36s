#include "update_install.h"
#include "update_config.h"
#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int sha256_file(const char *path, char out[65])
{
    char command[1200];
    char line[160];
    FILE *pipe;

    if (!path || !out) return -1;
    if (snprintf(command, sizeof(command), "sha256sum -- '%s'", path) >= (int)sizeof(command)) return -1;

    pipe = popen(command, "r");
    if (!pipe) return -1;
    if (!fgets(line, sizeof(line), pipe)) {
        pclose(pipe);
        return -1;
    }
    if (pclose(pipe) != 0) return -1;
    if (strlen(line) < 64) return -1;

    memcpy(out, line, 64);
    out[64] = '\0';
    return 0;
}

static int copy_file_synced(const char *src, const char *dst, mode_t mode)
{
    int in_fd = -1;
    int out_fd = -1;
    char buffer[65536];
    ssize_t n;
    int result = -1;

    in_fd = open(src, O_RDONLY);
    if (in_fd < 0) goto done;

    out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (out_fd < 0) goto done;

    while ((n = read(in_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(out_fd, buffer + written, (size_t)(n - written));
            if (w <= 0) goto done;
            written += w;
        }
    }
    if (n < 0) goto done;
    if (fchmod(out_fd, mode) != 0) goto done;
    if (fsync(out_fd) != 0) goto done;

    result = 0;

done:
    if (out_fd >= 0) close(out_fd);
    if (in_fd >= 0) close(in_fd);
    if (result != 0) unlink(dst);
    return result;
}

static int fsync_parent_dir(const char *path)
{
    char dir_path[PATH_MAX];
    char *slash;
    int fd;
    int rc;

    if (!path || strlen(path) >= sizeof(dir_path)) return -1;
    snprintf(dir_path, sizeof(dir_path), "%s", path);
    slash = strrchr(dir_path, '/');
    if (!slash) return -1;
    if (slash == dir_path) slash[1] = '\0';
    else *slash = '\0';

    fd = open(dir_path, O_RDONLY | O_DIRECTORY);
    if (fd < 0) return -1;
    rc = fsync(fd);
    close(fd);
    return rc;
}

int update_download_and_verify(const UpdateManifest *manifest,
                               const char *target_path,
                               char *status,
                               int status_size)
{
    CURL *curl = NULL;
    CURLcode rc;
    FILE *out = NULL;
    long http_code = 0;
    char actual_sha256[65];
    const char *ca;
    const char *cert;
    const char *key;
    const char *password;
    int result = -1;

    if (!status || status_size <= 0) return -1;
    status[0] = '\0';

    if (!manifest || !target_path || !manifest->binary_url[0] || strlen(manifest->sha256) != 64) {
        snprintf(status, status_size, "Ungueltige Update-Daten");
        return -1;
    }
    if (strncmp(manifest->binary_url, "https://", 8) != 0) {
        snprintf(status, status_size, "Update-URL muss HTTPS verwenden");
        return -1;
    }

    unlink(target_path);
    out = fopen(target_path, "wb");
    if (!out) {
        snprintf(status, status_size, "Zieldatei nicht schreibbar: %s", strerror(errno));
        return -1;
    }

    curl = curl_easy_init();
    if (!curl) {
        snprintf(status, status_size, "curl konnte nicht gestartet werden");
        goto done;
    }

    ca = update_config_ca_cert();
    cert = update_config_client_cert();
    key = update_config_client_key();
    password = update_config_client_key_password();

    curl_easy_setopt(curl, CURLOPT_URL, manifest->binary_url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "HoerspielPlayer-Update/1");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, update_config_verify_peer() ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, update_config_verify_host() ? 2L : 0L);
    if (ca && ca[0]) curl_easy_setopt(curl, CURLOPT_CAINFO, ca);
    if (cert && cert[0]) curl_easy_setopt(curl, CURLOPT_SSLCERT, cert);
    if (key && key[0]) curl_easy_setopt(curl, CURLOPT_SSLKEY, key);
    if (password && password[0]) curl_easy_setopt(curl, CURLOPT_KEYPASSWD, password);

    rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        snprintf(status, status_size, "Download fehlgeschlagen: %.120s", curl_easy_strerror(rc));
        goto done;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        snprintf(status, status_size, "Download HTTP %ld", http_code);
        goto done;
    }

    if (fflush(out) != 0 || fsync(fileno(out)) != 0) {
        snprintf(status, status_size, "Update-Datei konnte nicht gespeichert werden");
        goto done;
    }
    fclose(out);
    out = NULL;

    if (sha256_file(target_path, actual_sha256) != 0) {
        snprintf(status, status_size, "SHA256 konnte nicht berechnet werden");
        goto done;
    }
    if (strcasecmp(actual_sha256, manifest->sha256) != 0) {
        unlink(target_path);
        snprintf(status, status_size, "SHA256 stimmt nicht ueberein");
        goto done;
    }

    snprintf(status, status_size, "Download OK, SHA256 OK");
    result = 0;

done:
    if (curl) curl_easy_cleanup(curl);
    if (out) fclose(out);
    if (result != 0) unlink(target_path);
    return result;
}

int update_install_staged(const char *staged_path,
                          char *status,
                          int status_size)
{
    char current_path[PATH_MAX];
    char new_path[PATH_MAX];
    char old_path[PATH_MAX];
    struct stat st;
    ssize_t len;
    mode_t mode;
    int current_moved = 0;

    if (!status || status_size <= 0) return -1;
    status[0] = '\0';

    if (!staged_path || access(staged_path, R_OK) != 0) {
        snprintf(status, status_size, "Kein vorbereitetes Update gefunden");
        return -1;
    }

    len = readlink("/proc/self/exe", current_path, sizeof(current_path) - 1);
    if (len <= 0 || len >= (ssize_t)sizeof(current_path) - 1) {
        snprintf(status, status_size, "Programmpfad konnte nicht ermittelt werden");
        return -1;
    }
    current_path[len] = '\0';

    if (stat(current_path, &st) != 0) {
        snprintf(status, status_size, "Aktuelle Binary nicht gefunden");
        return -1;
    }

    if (snprintf(new_path, sizeof(new_path), "%s.new", current_path) >= (int)sizeof(new_path) ||
        snprintf(old_path, sizeof(old_path), "%s.old", current_path) >= (int)sizeof(old_path)) {
        snprintf(status, status_size, "Programmpfad ist zu lang");
        return -1;
    }

    mode = st.st_mode & 0777;
    mode |= S_IXUSR;

    unlink(new_path);
    if (copy_file_synced(staged_path, new_path, mode) != 0) {
        snprintf(status, status_size, "Update konnte nicht neben Binary kopiert werden");
        return -1;
    }

    unlink(old_path);
    if (rename(current_path, old_path) != 0) {
        unlink(new_path);
        snprintf(status, status_size, "Backup der aktuellen Binary fehlgeschlagen: %s", strerror(errno));
        return -1;
    }
    current_moved = 1;

    if (rename(new_path, current_path) != 0) {
        int saved_errno = errno;
        if (rename(old_path, current_path) == 0) {
            fsync_parent_dir(current_path);
        }
        unlink(new_path);
        snprintf(status, status_size, "Aktivierung fehlgeschlagen, Rollback ausgefuehrt: %s", strerror(saved_errno));
        return -1;
    }
    current_moved = 0;

    if (fsync_parent_dir(current_path) != 0) {
        if (rename(current_path, new_path) == 0 && rename(old_path, current_path) == 0) {
            fsync_parent_dir(current_path);
            unlink(new_path);
            snprintf(status, status_size, "Dateisystem-Sync fehlgeschlagen, Rollback ausgefuehrt");
        } else {
            snprintf(status, status_size, "Update aktiv, aber Verzeichnis-Sync fehlgeschlagen");
        }
        return -1;
    }

    unlink(staged_path);
    snprintf(status, status_size, "Update installiert; Neustart erforderlich");
    (void)current_moved;
    return 0;
}
