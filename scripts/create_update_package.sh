#!/usr/bin/env bash
set -euo pipefail

# Erstellt ein komplettes Update-Paket aus den Docker-Builds.
#
# Aufruf:
#   ./scripts/create_update_package.sh https://DEIN-SERVER
#
# Erwartet:
#   ./config.h
#   ./dist-r36s/hoerspiel_player
#   ./dist-batocera/hoerspiel_player

BASE_URL="${1:-${UPDATE_BASE_URL:-}}"

if [[ -z "$BASE_URL" ]]; then
    echo "Fehler: Base-URL fehlt."
    echo "Aufruf: $0 https://DEIN-SERVER"
    echo "oder:   UPDATE_BASE_URL=https://DEIN-SERVER make updatepackage"
    exit 1
fi

BASE_URL="${BASE_URL%/}"

CONFIG_FILE="config.h"
R36S_BINARY="dist-r36s/hoerspiel_player"
GPM2804_BINARY="dist-batocera/hoerspiel_player"

for file in "$CONFIG_FILE" "$R36S_BINARY" "$GPM2804_BINARY"; do
    if [[ ! -f "$file" ]]; then
        echo "Fehler: Datei nicht gefunden: $file"
        echo "Bitte zuerst beide Modelle bauen: make r36s && make gpm2804"
        exit 1
    fi
done

VERSION="$(sed -n 's/^#define APP_VERSION "\([^"]*\)".*/\1/p' "$CONFIG_FILE")"

if [[ -z "$VERSION" ]]; then
    echo "Fehler: APP_VERSION konnte nicht aus $CONFIG_FILE gelesen werden."
    exit 1
fi

OUTPUT_DIR="update"
VERSION_DIR="$OUTPUT_DIR/updates/$VERSION"
R36S_DIR="$VERSION_DIR/r36s"
GPM2804_DIR="$VERSION_DIR/gpm2804"
VERSION_JSON="$OUTPUT_DIR/version.json"

echo "Erstelle Update-Paket fuer Version $VERSION ..."

rm -rf "$OUTPUT_DIR"
mkdir -p "$R36S_DIR" "$GPM2804_DIR"

cp "$R36S_BINARY" "$R36S_DIR/hoerspiel_player"
cp "$GPM2804_BINARY" "$GPM2804_DIR/hoerspiel_player"

R36S_SHA256="$(sha256sum "$R36S_DIR/hoerspiel_player" | awk '{print $1}')"
GPM2804_SHA256="$(sha256sum "$GPM2804_DIR/hoerspiel_player" | awk '{print $1}')"

cat > "$VERSION_JSON" <<EOF
{
  "version": "$VERSION",
  "r36s": {
    "url": "$BASE_URL/updates/$VERSION/r36s/hoerspiel_player",
    "sha256": "$R36S_SHA256"
  },
  "gpm2804": {
    "url": "$BASE_URL/updates/$VERSION/gpm2804/hoerspiel_player",
    "sha256": "$GPM2804_SHA256"
  }
}
EOF

echo
echo "Update-Paket fertig:"
echo "  $VERSION_JSON"
echo "  $R36S_DIR/hoerspiel_player"
echo "  $GPM2804_DIR/hoerspiel_player"
echo
echo "SHA256 R36S:    $R36S_SHA256"
echo "SHA256 GPM2804: $GPM2804_SHA256"
echo
find "$OUTPUT_DIR" -type f -print
