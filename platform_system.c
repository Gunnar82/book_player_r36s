#include <stdlib.h>
#include <string.h>

/*
 * Batocera stellt auf dem Zielsystem kein busctl/loginctl bereit.
 * Der bestehende gemeinsame Quellcode ruft fuer PowerOff noch den R36S-
 * logind-Befehl ueber system() auf. Im Batocera-Link wird system() deshalb
 * per GNU-ld --wrap abgefangen. Nur dieser eine PowerOff-Aufruf wird auf den
 * auf dem GPM2804 getesteten nativen Shutdown umgeleitet; alle anderen
 * system()-Aufrufe gehen unveraendert an libc weiter.
 */
extern int __real_system(const char *command);

int __wrap_system(const char *command)
{
    if (command && strstr(command, "org.freedesktop.login1.Manager PowerOff"))
        return __real_system("/sbin/shutdown -P -h now");

    return __real_system(command);
}
