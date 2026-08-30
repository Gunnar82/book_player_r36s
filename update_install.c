#include "update_install.h"
#include "update_config.h"
#include <curl/curl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
