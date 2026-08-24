#include <stdlib.h>
#include <string.h>

/*
 * Batocera auf dem GPM2804 stellt kein busctl/loginctl bereit.
 * main.c verwendet historisch noch den R36S-logind-PowerOff-Aufruf.
 * Bis main.c in einem spaeteren, separat testbaren Schritt entkoppelt wird,
 * faengt dieser kleine Adapter genau diesen einen system()-Aufruf ab.
 * Alle anderen system()-Aufrufe werden unveraendert an libc weitergereicht.
 */
extern int __real_system(const char *command);

int __wrap_system(const char *command)
{
    if (command && strstr(command, "org.freedesktop.login1.Manager PowerOff"))
        return __real_system("/sbin/shutdown -P -h now");

    return __real_system(command);
}
