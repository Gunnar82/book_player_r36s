FROM debian:bookworm

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    patch \
    libsdl2-dev \
    libsdl2-mixer-dev \
    libsdl2-ttf-dev \
    libcurl4-openssl-dev \
    ca-certificates \
    pkg-config \
    binutils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY *.c *.h ./
COPY screens ./screens
COPY patches ./patches

# develop-0.2 basiert noch auf dem stabilen 0.1.14-Quellstand plus Patch-Stack.
# 01-core.patch passt wegen spaeterer 0.1-Aenderungen nicht mehr an jeder Stelle
# sauber auf main.c. Die passenden Hunks werden uebernommen; die wenigen
# verbleibenden Core-Aenderungen werden darunter idempotent ergaenzt.
RUN patch -p1 --forward < patches/01-core.patch || true

RUN grep -q '#include "screens/systemmenu.h"' main.c || \
      sed -i '/#include "screens\/buttondebug.h"/a #include "screens/systemmenu.h"\n#include "screens/downloadbrowser.h"' main.c; \
    grep -q 'load_download_config();' main.c || \
      sed -i '/load_playback_config();/a\    load_download_config();' main.c; \
    sed -i 's/if (e.jbutton.button == BUTTON_X) { toggle_display(); continue; }/if (e.jbutton.button == BUTTON_X) { screen = SCREEN_SYSTEM_MENU; continue; }/' main.c; \
    grep -q 'case SCREEN_SYSTEM_MENU: systemmenu_handle_event' main.c || \
      sed -i '/case SCREEN_BUTTON_DEBUG: buttondebug_handle_event(&screen_ctx, &e); break;/a\                case SCREEN_SYSTEM_MENU: systemmenu_handle_event(&screen_ctx, &e); break;\n                case SCREEN_DOWNLOADS: downloadbrowser_handle_event(&screen_ctx, &e); break;' main.c; \
    grep -q 'case SCREEN_SYSTEM_MENU: systemmenu_render' main.c || \
      sed -i '/case SCREEN_BUTTON_DEBUG: buttondebug_render(&render_ctx); break;/a\            case SCREEN_SYSTEM_MENU: systemmenu_render(&render_ctx); break;\n            case SCREEN_DOWNLOADS: downloadbrowser_render(&render_ctx); break;' main.c; \
    grep -q 'SCREEN_SYSTEM_MENU' screens.h || \
      sed -i 's/SCREEN_BUTTON_DEBUG = 5/SCREEN_BUTTON_DEBUG = 5,\n    SCREEN_SYSTEM_MENU = 6,\n    SCREEN_DOWNLOADS = 7/' screens.h

# Die restlichen 0.2-Patches sind auf dem aktuellen Basestand stabil.
RUN patch -p1 --forward < patches/02-storage.patch && \
    patch -p1 --forward < patches/03-ui.patch && \
    if [ ! -f download.c ]; then patch -p1 < patches/04-download-core.patch; fi && \
    if [ ! -f screens/downloadbrowser.c ]; then patch -p1 < patches/05-download-ui.patch; fi && \
    patch -p1 --forward < patches/06-config-path-info.patch

# Buildnummer wird an genau einer Stelle gesetzt. So kann der historische
# Patch-Stack erhalten bleiben, ohne Versionspatches voneinander abhaengig zu machen.
RUN sed -i 's/^#define APP_VERSION .*/#define APP_VERSION "0.2.0-dev4"/' config.h

# Vor dem Kompilieren pruefen wir die Symbole, die bei einem unvollstaendig
# angewendeten Patch-Stack fehlen wuerden. Der Build bricht dann mit einer
# verstaendlichen Ursache ab statt spaeter mit einer Kaskade von C-Fehlern.
RUN grep -q 'extern char download_base_url' storage.h && \
    grep -q 'extern char download_target_path' storage.h && \
    grep -q 'SCREEN_SYSTEM_MENU' screens.h && \
    grep -q 'load_download_config();' main.c && \
    grep -q 'systemmenu_handle_event' main.c && \
    grep -q 'downloadbrowser_handle_event' main.c && \
    grep -q 'get_storage_config_path' screens/systeminfo.c

# Zielsystem DarkOS ist AArch64. SDL2, curl usw. kommen vom Zielsystem.
RUN gcc -o hoerspiel_player \
    main.c state.c backlight.c battery.c led.c scanner.c audio.c ui.c \
    storage.c systemstats.c media_keys.c download.c \
    screens/menu.c screens/tracks.c screens/player.c \
    screens/systemmenu.c screens/systeminfo.c screens/buttondebug.c \
    screens/downloadbrowser.c \
    $(pkg-config --cflags --libs sdl2 SDL2_mixer SDL2_ttf libcurl)

RUN readelf -h /build/hoerspiel_player | grep -E 'Class:|Machine:' && \
    grep 'APP_VERSION "0.2.0-dev4"' /build/config.h

CMD ["true"]
