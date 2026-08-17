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

# 0.2 liegt auf dem Entwicklungsbranch als Patch-Stack auf dem 0.1.14-Quellstand.
RUN if [ ! -f download.c ]; then \
      for p in patches/*.patch; do patch -p1 < "$p"; done; \
    fi

# Das Zielsystem DarkOS ist AArch64. Die Laufzeitbibliotheken (SDL2, curl usw.)
# kommen vom Zielsystem und werden nicht aus dem Build-Container gebuendelt.
RUN gcc -o hoerspiel_player \
    main.c state.c backlight.c battery.c led.c scanner.c audio.c ui.c \
    storage.c systemstats.c media_keys.c download.c \
    screens/menu.c screens/tracks.c screens/player.c \
    screens/systemmenu.c screens/systeminfo.c screens/buttondebug.c \
    screens/downloadbrowser.c \
    $(pkg-config --cflags --libs sdl2 SDL2_mixer SDL2_ttf libcurl)

RUN readelf -h /build/hoerspiel_player | grep -E 'Class:|Machine:'

CMD ["true"]
