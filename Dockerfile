FROM debian:bookworm AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential make libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev \
    libcurl4-openssl-dev libsystemd-dev libqrencode-dev ca-certificates \
    pkg-config binutils && rm -rf /var/lib/apt/lists/*
WORKDIR /build
COPY *.c *.h ./
COPY Makefile.r36s ./Makefile
COPY screens ./screens
RUN grep 'APP_VERSION "0.3.59"' config.h && \
    grep -q 'extern char download_base_url' storage.h && \
    grep -q 'extern int display_timeout_seconds' state.h && \
    grep -q 'mpris_bridge_init' mpris_bridge.h && \
    grep -q 'media_feedback_show' media_feedback.h && \
    grep -q 'media_capable' media_keys.h && \
    grep -q 'SCREEN_DOWNLOADS' screens.h && \
    test -f update_check.c && test -f update_check.h && test -f screens/updatesettings.c && \
    test -f download.c && test -f mpris_bridge.c && test -f bluetooth.c && \
    test -f bluetooth_audio_sink.c && test -f hfp_gateway.c && test -f pbap_phonebook.c && \
    test -f battery_bluez.c && grep -q 'org.bluez.Device1' bluetooth.c && \
    test -f screens/bluetooth.c && test -f screens/downloadsettings.c && \
    test -f screens/streamsettings.c && test -f media_feedback.c && test -f app_log.c && \
    test -f screens/downloadbrowser.c && test -f screens/logview.c
RUN make clean && make
RUN readelf -h /build/hoerspiel_player | grep -E 'Class:|Machine:'
RUN mkdir -p /dist/lib && cp /build/hoerspiel_player /dist/hoerspiel_player
RUN ldd /build/hoerspiel_player | awk '/=> \// { print $3 } /^\// { print $1 }' | sort -u > /tmp/libs.txt && while read lib; do cp -L "$lib" /dist/lib/; done < /tmp/libs.txt
RUN cp -L /lib/ld-linux-aarch64.so.1 /dist/lib/ld-linux-aarch64.so.1
RUN echo "=== Binary ===" && readelf -h /dist/hoerspiel_player | grep -E 'Class:|Machine:' && echo "=== Libraries ===" && ls -lah /dist/lib && echo "=== Runtime dependencies ===" && ldd /dist/hoerspiel_player
FROM scratch AS export
COPY --from=build /dist /
