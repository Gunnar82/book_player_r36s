FROM debian:bookworm

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    libsdl2-dev \
    libsdl2-mixer-dev \
    libsdl2-ttf-dev \
    libcurl4-openssl-dev \
    ca-certificates \
    pkg-config \
    binutils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# develop-0.2 enthaelt ab dev5 vollstaendig eingearbeitete Quellen.
# Es werden keine historischen Patch-Dateien mehr waehrend des Builds angewendet.
COPY *.c *.h ./
COPY screens ./screens

RUN grep 'APP_VERSION "0.2.0-dev6"' config.h && \
    grep -q 'extern char download_base_url' storage.h && \
    grep -q 'SCREEN_DOWNLOADS' screens.h && \
    test -f download.c && \
    test -f screens/downloadbrowser.c

RUN gcc -o hoerspiel_player \
    main.c state.c backlight.c battery.c led.c scanner.c audio.c ui.c \
    storage.c systemstats.c media_keys.c download.c \
    screens/menu.c screens/tracks.c screens/player.c \
    screens/systemmenu.c screens/systeminfo.c screens/buttondebug.c \
    screens/downloadbrowser.c \
    $(pkg-config --cflags --libs sdl2 SDL2_mixer SDL2_ttf libcurl)

RUN readelf -h /build/hoerspiel_player | grep -E 'Class:|Machine:'

CMD ["true"]
