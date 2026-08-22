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

RUN grep 'APP_VERSION "0.2.35-input-config"' config.h && \
    grep -q 'extern char download_base_url' storage.h && \
    grep -q 'extern int display_timeout_seconds' state.h && \
    grep -q 'mpris_bridge_init' mpris_bridge.h && \
    grep -q 'media_feedback_show' media_feedback.h && \
    grep -q 'media_capable' media_keys.h && \
    grep -q 'SCREEN_DOWNLOADS' screens.h && \
    test -f download.c && \
    test -f mpris_bridge.c && \
    test -f bluetooth.c && \
    test -f hfp_gateway.c && \
    test -f pbap_phonebook.c && \
    test -f battery_bluez.c && \
    grep -q 'org.bluez.Device1' bluetooth.c && \
    test -f screens/bluetooth.c && \
    test -f screens/downloadsettings.c && \
    test -f media_feedback.c && \
    test -f app_log.c && \
    test -f screens/downloadbrowser.c && \
    test -f screens/logview.c

RUN gcc -o hoerspiel_player \
    main.c state.c backlight.c battery.c battery_bluez.c led.c scanner.c audio.c ui.c \
    storage.c systemstats.c media_keys.c media_feedback.c app_log.c input_config.c mpris_bridge.c bluetooth.c hfp_gateway.c pbap_phonebook.c download.c \
    screens/menu.c screens/tracks.c screens/player.c \
    screens/systemmenu.c screens/systeminfo.c screens/buttondebug.c \
    screens/downloadbrowser.c screens/logview.c screens/bluetooth.c screens/downloadsettings.c \
    $(pkg-config --cflags --libs sdl2 SDL2_mixer SDL2_ttf libcurl libsystemd)

RUN readelf -h /build/hoerspiel_player | grep -E 'Class:|Machine:'

CMD ["true"]
