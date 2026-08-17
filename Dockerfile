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

# develop-0.2 basiert auf dem stabilen 0.1.14-Quellstand.
# Die Kern-/Storage-/UI-Patches muessen immer angewendet werden.
# download.c/.h und downloadbrowser.c/.h koennen im Branch bereits direkt
# vorhanden sein; deren New-File-Patches werden dann uebersprungen.
RUN patch -p1 < patches/01-core.patch && \
    patch -p1 < patches/02-storage.patch && \
    patch -p1 < patches/03-ui.patch && \
    if [ ! -f download.c ]; then patch -p1 < patches/04-download-core.patch; fi && \
    if [ ! -f screens/downloadbrowser.c ]; then patch -p1 < patches/05-download-ui.patch; fi && \
    patch -p1 < patches/06-config-path-info.patch && \
    patch -p1 < patches/07-version-dev2.patch && \
    patch -p1 < patches/08-version-dev3.patch

# Zielsystem DarkOS ist AArch64. SDL2, curl usw. kommen vom Zielsystem.
RUN gcc -o hoerspiel_player \
    main.c state.c backlight.c battery.c led.c scanner.c audio.c ui.c \
    storage.c systemstats.c media_keys.c download.c \
    screens/menu.c screens/tracks.c screens/player.c \
    screens/systemmenu.c screens/systeminfo.c screens/buttondebug.c \
    screens/downloadbrowser.c \
    $(pkg-config --cflags --libs sdl2 SDL2_mixer SDL2_ttf libcurl)

RUN readelf -h /build/hoerspiel_player | grep -E 'Class:|Machine:' && \
    grep 'APP_VERSION "0.2.0-dev3"' /build/config.h

CMD ["true"]
