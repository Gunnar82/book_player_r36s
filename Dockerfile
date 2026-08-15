FROM --platform=linux/arm/v7 debian:bookworm

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    libsdl2-dev \
    libsdl2-mixer-dev \
    libsdl2-ttf-dev \
    pax-utils \
    patchelf \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# GitHub-Repository: Sources liegen direkt im Root, screens/ als Unterordner.
COPY *.c *.h ./
COPY screens ./screens

RUN gcc -o hoerspiel_player \
    main.c state.c backlight.c battery.c led.c scanner.c audio.c ui.c \
    storage.c systemstats.c media_keys.c \
    screens/menu.c screens/tracks.c screens/player.c \
    screens/systemmenu.c screens/systeminfo.c screens/buttondebug.c \
    $(pkg-config --cflags --libs sdl2 SDL2_mixer SDL2_ttf) \
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
