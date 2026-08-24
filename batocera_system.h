#ifndef BATOCERA_SYSTEM_H
#define BATOCERA_SYSTEM_H

#include <stdlib.h>
#include <string.h>

/*
 * Batocera does not provide busctl/loginctl on the target image.
 * The player still uses the R36S logind command in shared source code,
 * so the Batocera build redirects only that PowerOff request to the
 * native shutdown command. All other system() calls are passed through.
 */
static inline int batocera_system(const char *command)
{
    if (command && strstr(command, "org.freedesktop.login1.Manager PowerOff"))
        return system("/sbin/shutdown -P -h now");

    return system(command);
}

#define system(command) batocera_system(command)

#endif
