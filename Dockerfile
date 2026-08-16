FROM --platform=linux/arm/v7 debian:bookworm

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    patch \
    libsdl2-dev \
    libsdl2-mixer-dev \
    libsdl2-ttf-dev \
    libcurl4-openssl-dev \
    ca-certificates \
    pax-utils \
    patchelf \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY *.c *.h ./
COPY screens ./screens
COPY patches ./patches

# Auf dem GitHub-Entwicklungsbranch liegt 0.2 als Patch-Stack auf 0.1.14.
# Im Download-ZIP sind die Patches bereits in die Quellen eingearbeitet.
RUN if [ ! -f download.c ]; then \
      for p in patches/*.patch; do patch -p1 < "$p"; done; \
    fi

RUN gcc -o hoerspiel_player \
    main.c state.c backlight.c battery.c led.c scanner.c audio.c ui.c \
    storage.c systemstats.c media_keys.c download.c \
    screens/menu.c screens/tracks.c screens/player.c \
    screens/systemmenu.c screens/systeminfo.c screens/buttondebug.c \
    screens/downloadbrowser.c \
    $(pkg-config --cflags --libs sdl2 SDL2_mixer SDL2_ttf libcurl) \
    -Wl,-rpath,'$ORIGIN/lib'

RUN mkdir -p /build/lib && \
    lddtree -l /build/hoerspiel_player \
    | grep '^/' \
    | sort -u \
    | grep -Ev '/(libc.so|libm.so|libpthread.so|ld-linux)' \
    | while read lib; do \
        case "$lib" in \
            /build/lib/*) ;; \
            *) cp -Lv "$lib" /build/lib/ || true ;; \
        esac; \
    done

CMD ["true"]
