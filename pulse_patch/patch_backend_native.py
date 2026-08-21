#!/usr/bin/env python3
from pathlib import Path
import sys

if len(sys.argv) != 2:
    print('Usage: patch_backend_native.py /path/to/src/modules/bluetooth/backend-native.c')
    sys.exit(2)

p=Path(sys.argv[1])
s=p.read_text()
if 'hoerspiel_forward_dial' in s:
    print('already patched')
    sys.exit(0)

needle='#include <sys/types.h>'
if needle not in s:
    raise SystemExit('unexpected PulseAudio source: sys/types.h include not found')
s=s.replace(needle, needle+'\n#include <sys/socket.h>\n#include <sys/un.h>\n#include <unistd.h>',1)

helper=r'''
/* Hoerspiel Player: forward HFP ATD commands without changing HFP behaviour. */
static void hoerspiel_forward_dial(const char *buf) {
    int fd;
    struct sockaddr_un sa;
    char path[sizeof(sa.sun_path)];
    const char *runtime;
    const char *p;
    char msg[96];
    size_t n = 0;

    if (!buf || !pa_startswith(buf, "ATD"))
        return;

    p = buf + 3;
    while (*p && *p != ';' && *p != '\r' && *p != '\n' && n + 6 < sizeof(msg)) {
        if ((*p >= '0' && *p <= '9') || *p == '+' || *p == '#' || *p == '*')
            msg[5 + n++] = *p;
        p++;
    }
    if (!n)
        return;
    memcpy(msg, "DIAL ", 5);
    msg[5 + n] = '\0';

    runtime = getenv("XDG_RUNTIME_DIR");
    if (runtime && *runtime)
        pa_snprintf(path, sizeof(path), "%s/hoerspiel-player-hfp.sock", runtime);
    else
        pa_snprintf(path, sizeof(path), "/run/user/%lu/hoerspiel-player-hfp.sock", (unsigned long) getuid());

    fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (fd < 0)
        return;
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    pa_strlcpy(sa.sun_path, path, sizeof(sa.sun_path));
    (void) sendto(fd, msg, 5 + n, MSG_DONTWAIT, (struct sockaddr *) &sa, sizeof(sa));
    close(fd);
}
'''
marker='static bool hfp_rfcomm_handle('
pos=s.find(marker)
if pos < 0:
    raise SystemExit('unexpected PulseAudio source: hfp_rfcomm_handle not found')
s=s[:pos]+helper+'\n'+s[pos:]
start=s.find(marker, pos+len(helper))
brace=s.find('{', start)
if brace < 0:
    raise SystemExit('unexpected PulseAudio source: function brace not found')
s=s[:brace+1]+'\n    hoerspiel_forward_dial(buf);'+s[brace+1:]
p.write_text(s)
print('patched', p)
