#include "update_config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int updates_enabled = 0;
int update_use_download_tls = 1;
int update_verify_peer = 1;
int update_verify_host = 1;
char update_base_url[UPDATE_URL_LEN] = "";
char update_ca_cert[STORAGE_PATH_LEN] = "";
char update_client_cert[STORAGE_PATH_LEN] = "";
char update_client_key[STORAGE_PATH_LEN] = "";
char update_client_key_password[256] = "";

static void trim_update(char *s)
{
    char *start;
    size_t len;

    if (!s) {
        return;
    }
    start = s;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != s) {
        memmove(s, start, strlen(start) + 1);
    }
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}

int update_config_ensure_section(void)
{
    const char *path = get_storage_config_path();
    FILE *fp = fopen(path, "r");
    char line[1400];
    int found = 0;

    if (!fp) {
        return -1;
    }
    while (fgets(line, sizeof(line), fp)) {
        trim_update(line);
        if (!strcmp(line, "[updates]")) {
            found = 1;
            break;
        }
    }
    fclose(fp);
    if (found) {
        return 0;
    }

    fp = fopen(path, "a");
    if (!fp) {
        return -1;
    }
    if (fprintf(fp,
                "\n[updates]\n"
                "# Self-Updates sind standardmaessig deaktiviert.\n"
                "enabled=0\n"
                "# HTTPS-Basis-URL fuer latest.json und Update-Dateien.\n"
                "base_url=\n"
                "# 1 = TLS-Werte aus [download], 0 = eigene Werte unten.\n"
                "use_download_tls=1\n"
                "verify_peer=1\n"
                "verify_host=1\n"
                "ca_cert=\n"
                "client_cert=\n"
                "client_key=\n"
                "client_key_password=\n") < 0) {
        fclose(fp);
        return -1;
    }
    if (fclose(fp) != 0) {
        return -1;
    }
    return 0;
}

void update_config_load(void)
{
    FILE *fp;
    char line[1400];
    int in_updates = 0;

    updates_enabled = 0;
    update_use_download_tls = 1;
    update_verify_peer = 1;
    update_verify_host = 1;
    update_base_url[0] = '\0';
    update_ca_cert[0] = '\0';
    update_client_cert[0] = '\0';
    update_client_key[0] = '\0';
    update_client_key_password[0] = '\0';

    fp = fopen(get_storage_config_path(), "r");
    if (!fp) {
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *eq;
        trim_update(line);
        if (!line[0] || line[0] == '#' || line[0] == ';') {
            continue;
        }
        if (line[0] == '[') {
            in_updates = !strcmp(line, "[updates]");
            continue;
        }
        if (!in_updates) {
            continue;
        }
        eq = strchr(line, '=');
        if (!eq) {
            continue;
        }
        *eq++ = '\0';
        trim_update(line);
        trim_update(eq);

        if (!strcmp(line, "enabled")) {
            updates_enabled = atoi(eq) ? 1 : 0;
        } else if (!strcmp(line, "base_url")) {
            snprintf(update_base_url, sizeof(update_base_url), "%s", eq);
        } else if (!strcmp(line, "use_download_tls")) {
            update_use_download_tls = atoi(eq) ? 1 : 0;
        } else if (!strcmp(line, "verify_peer")) {
            update_verify_peer = atoi(eq) ? 1 : 0;
        } else if (!strcmp(line, "verify_host")) {
            update_verify_host = atoi(eq) ? 1 : 0;
        } else if (!strcmp(line, "ca_cert")) {
            snprintf(update_ca_cert, sizeof(update_ca_cert), "%s", eq);
        } else if (!strcmp(line, "client_cert")) {
            snprintf(update_client_cert, sizeof(update_client_cert), "%s", eq);
        } else if (!strcmp(line, "client_key")) {
            snprintf(update_client_key, sizeof(update_client_key), "%s", eq);
        } else if (!strcmp(line, "client_key_password")) {
            snprintf(update_client_key_password, sizeof(update_client_key_password), "%s", eq);
        }
    }
    fclose(fp);
}

const char *update_config_ca_cert(void)
{
    return update_use_download_tls ? download_ca_cert : update_ca_cert;
}

const char *update_config_client_cert(void)
{
    return update_use_download_tls ? download_client_cert : update_client_cert;
}

const char *update_config_client_key(void)
{
    return update_use_download_tls ? download_client_key : update_client_key;
}

const char *update_config_client_key_password(void)
{
    return update_use_download_tls ? download_client_key_password : update_client_key_password;
}

int update_config_verify_peer(void)
{
    return update_use_download_tls ? download_verify_peer : update_verify_peer;
}

int update_config_verify_host(void)
{
    return update_use_download_tls ? download_verify_host : update_verify_host;
}
