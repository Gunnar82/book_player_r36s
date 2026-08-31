#include "update_check.h"
#include "update_config.h"
#include <curl/curl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t size;
} UpdateBuffer;

static size_t update_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    UpdateBuffer *buffer = (UpdateBuffer *)userdata;
    size_t bytes = size * nmemb;
    char *next = realloc(buffer->data, buffer->size + bytes + 1);

    if (!next) {
        return 0;
    }
    buffer->data = next;
    memcpy(buffer->data + buffer->size, ptr, bytes);
    buffer->size += bytes;
    buffer->data[buffer->size] = '\0';
    return bytes;
}

static int json_string(const char *json, const char *key, char *out, size_t out_size)
{
    char needle[96];
    const char *p;
    const char *end;
    size_t len;

    if (!json || !key || !out || out_size == 0) {
        return 0;
    }
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    p = strstr(json, needle);
    if (!p) {
        return 0;
    }
    p += strlen(needle);
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    if (*p != ':') {
        return 0;
    }
    p++;
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    if (*p != '"') {
        return 0;
    }
    p++;
    end = p;
    while (*end && *end != '"') {
        if (*end == '\\' && end[1]) {
            end += 2;
        } else {
            end++;
        }
    }
    if (*end != '"') {
        return 0;
    }
    len = (size_t)(end - p);
    if (len >= out_size) {
        return 0;
    }
    memcpy(out, p, len);
    out[len] = '\0';
    return 1;
}

static int platform_manifest(const char *json, UpdateManifest *manifest)
{
    const char *platform;
    const char *section;
    const char *end;
    char block[4096];
    size_t len;

#if defined(BUILD_BATOCERA)
    platform = "gpm2804";
#else
    platform = "r36s";
#endif

    if (!json_string(json, "version", manifest->version, sizeof(manifest->version))) {
        return 0;
    }
    section = strstr(json, platform);
    if (!section) {
        return 0;
    }
    section = strchr(section, '{');
    if (!section) {
        return 0;
    }
    end = strchr(section, '}');
    if (!end) {
        return 0;
    }
    len = (size_t)(end - section + 1);
    if (len >= sizeof(block)) {
        return 0;
    }
    memcpy(block, section, len);
    block[len] = '\0';
    if (!json_string(block, "url", manifest->binary_url, sizeof(manifest->binary_url))) {
        return 0;
    }
    if (!json_string(block, "sha256", manifest->sha256, sizeof(manifest->sha256))) {
        return 0;
    }
    return 1;
}

int update_version_compare(const char *a, const char *b)
{
    const char *pa = a;
    const char *pb = b;

    while ((pa && *pa) || (pb && *pb)) {
        long va = 0;
        long vb = 0;
        char *ea = NULL;
        char *eb = NULL;

        if (pa && *pa) {
            va = strtol(pa, &ea, 10);
        }
        if (pb && *pb) {
            vb = strtol(pb, &eb, 10);
        }
        if (va < vb) {
            return -1;
        }
        if (va > vb) {
            return 1;
        }
        if (!ea || ea == pa) {
            pa = NULL;
        } else {
            pa = *ea == '.' ? ea + 1 : ea;
        }
        if (!eb || eb == pb) {
            pb = NULL;
        } else {
            pb = *eb == '.' ? eb + 1 : eb;
        }
    }
    return 0;
}

int update_check_latest(UpdateManifest *manifest, char *status, int status_size)
{
    CURL *curl;
    CURLcode rc;
    UpdateBuffer buffer = {0};
    char manifest_url[UPDATE_CHECK_URL_LEN + 32];
    const char *ca;
    const char *cert;
    const char *key;
    const char *pass;
    long http_code = 0;
    size_t base_len;

    if (!manifest || !status || status_size <= 0) {
        return -1;
    }
    memset(manifest, 0, sizeof(*manifest));
    status[0] = '\0';
    if (!updates_enabled) {
        snprintf(status, (size_t)status_size, "Updates deaktiviert");
        return -1;
    }
    if (strncmp(update_base_url, "https://", 8) != 0) {
        snprintf(status, (size_t)status_size, "HTTPS Update-URL fehlt");
        return -1;
    }

    base_len = strlen(update_base_url);
    snprintf(manifest_url, sizeof(manifest_url), "%s%slatest.json", update_base_url,
             base_len > 0 && update_base_url[base_len - 1] == '/' ? "" : "/");

    curl = curl_easy_init();
    if (!curl) {
        snprintf(status, (size_t)status_size, "curl init fehlgeschlagen");
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, manifest_url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, update_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "HoerspielPlayer-Update/1");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, update_config_verify_peer() ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, update_config_verify_host() ? 2L : 0L);

    ca = update_config_ca_cert();
    cert = update_config_client_cert();
    key = update_config_client_key();
    pass = update_config_client_key_password();
    if (ca && ca[0]) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca);
    }
    if (cert && cert[0]) {
        curl_easy_setopt(curl, CURLOPT_SSLCERT, cert);
    }
    if (key && key[0]) {
        curl_easy_setopt(curl, CURLOPT_SSLKEY, key);
    }
    if (pass && pass[0]) {
        curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pass);
    }

    rc = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    if (rc != CURLE_OK) {
        snprintf(status, (size_t)status_size, "Netzwerk/TLS: %s", curl_easy_strerror(rc));
        free(buffer.data);
        return -1;
    }
    if (http_code != 200) {
        snprintf(status, (size_t)status_size, "HTTP %ld", http_code);
        free(buffer.data);
        return -1;
    }
    if (!buffer.data || !platform_manifest(buffer.data, manifest)) {
        snprintf(status, (size_t)status_size, "latest.json ungueltig");
        free(buffer.data);
        return -1;
    }
    free(buffer.data);
    snprintf(status, (size_t)status_size, "Version %s gefunden", manifest->version);
    return 0;
}
