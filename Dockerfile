FROM debian:bookworm

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    libsdl2-dev \
    libsdl2-mixer-dev \
    libsdl2-ttf-dev \
    libcurl4-openssl-dev \
    libsystemd-dev \
    ca-certificates \
    pkg-config \
    binutils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Seit 0.2 liegen alle Aenderungen direkt in den Quelldateien.
# Es werden keine historischen Patch-Dateien mehr waehrend des Builds angewendet.
COPY *.c *.h ./
COPY screens ./screens

RUN grep 'APP_VERSION "0.2.11"' config.h && \
    grep -q 'extern char download_base_url' storage.h && \
    grep -q 'extern int display_timeout_seconds' state.h && \
    grep -q 'mpris_bridge_init' mpris_bridge.h && \
    grep -q 'media_feedback_show' media_feedback.h && \
    grep -q 'media_capable' media_keys.h && \
    grep -q 'SCREEN_DOWNLOADS' screens.h && \
    test -f download.c && \
    test -f mpris_bridge.c && \
    test -f media_feedback.c && \
    test -f screens/downloadbrowser.c

RUN gcc -o hoerspiel_player \
    main.c state.c backlight.c battery.c led.c scanner.c audio.c ui.c \
    storage.c systemstats.c media_keys.c media_feedback.c mpris_bridge.c download.c \
    screens/menu.c screens/tracks.c screens/player.c \
    screens/systemmenu.c screens/systeminfo.c screens/buttondebug.c \
    screens/downloadbrowser.c \
    $(pkg-config --cflags --libs sdl2 SDL2_mixer SDL2_ttf libcurl libsystemd)

RUN readelf -h /build/hoerspiel_player | grep -E 'Class:|Machine:'

CMD ["true"]
