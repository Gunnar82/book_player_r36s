#include "output_volume.h"
#include "app_log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define AMIXER "/usr/bin/amixer"

static int clamp_percent(int value)
{
    if(value < 0) return 0;
    if(value > 100) return 100;
    return value;
}

int output_volume_get(int *percent)
{
    if(!percent || access(AMIXER, X_OK) != 0) return -1;
    FILE *fp = popen(AMIXER " get Master 2>/dev/null", "r");
    if(!fp) return -1;

    char line[512];
    int found = -1;
    while(fgets(line, sizeof(line), fp)) {
        char *pct = strchr(line, '%');
        if(!pct) continue;
        char *p = pct;
        while(p > line && isdigit((unsigned char)p[-1])) p--;
        if(p < pct) { found = atoi(p); break; }
    }
    pclose(fp);
    if(found < 0) return -1;
    *percent = clamp_percent(found);
    return 0;
}

int output_volume_set(int percent)
{
    if(access(AMIXER, X_OK) != 0) return -1;
    percent = clamp_percent(percent);
    char value[16];
    snprintf(value, sizeof(value), "%d%%", percent);

    pid_t pid = fork();
    if(pid < 0) return -1;
    if(pid == 0) {
        execl(AMIXER, AMIXER, "set", "Master", value, (char *)NULL);
        _exit(127);
    }

    int status = 0;
    if(waitpid(pid, &status, 0) < 0) return -1;
    if(WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        app_logf("Ausgangslautstaerke: Master -> %d%%", percent);
        return 0;
    }
    app_logf("Ausgangslautstaerke: Master konnte nicht gesetzt werden");
    return -1;
}

int output_volume_change(int delta, int *percent)
{
    int current;
    if(output_volume_get(&current) != 0) return -1;
    current = clamp_percent(current + delta);
    if(output_volume_set(current) != 0) return -1;
    if(percent) *percent = current;
    return 0;
}
