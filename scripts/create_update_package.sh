#!/usr/bin/env bash
set -euo pipefail

# Erstellt ein ZIP fuer den Update-Server.
#
# Aufruf:
#   ./scripts/create_update_package.sh https://DEIN-SERVER
#
# Ergebnis:
#   updates/update-<VERSION>.zip
#
# Inhalt des ZIPs:
#   updates/
#     latest.json
#     <VERSION>/
#       r36s/hoerspiel_player
#       gpm2804/hoerspiel_player

BASE_URL="${1:-${UPDATE_BASE_URL:-}}"

if [[ -z "$BASE_URL" ]]; then
    echo "Fehler: Base-URL fehlt."
    echo "Aufruf: $0 https://DEIN-SERVER"
    echo "oder:   make updatepackage UPDATE_BASE_URL=https://DEIN-SERVER"
    exit 1
fi

BASE_URL="${BASE_URL%/}"

CONFIG_FILE="config.h"
R36S_BINARY="dist-r36s/hoerspiel_player"
GPM2804_BINARY="dist-batocera/hoerspiel_player"

for file in "$CONFIG_FILE" "$R36S_BINARY" "$GPM2804_BINARY"; do
    if [[ ! -f "$file" ]]; then
        echo "Fehler: Datei nicht gefunden: $file"
        echo "Bitte zuerst beide Modelle bauen."
        exit 1
    fi
done

if ! command -v zip >/dev/null 2>&1; then
    echo "Fehler: 'zip' ist nicht installiert."
    exit 1
fi

VERSION="$(sed -n 's/^#define APP_VERSION "\([^"]*\)".*/\1/p' "$CONFIG_FILE")"

if [[ -z "$VERSION" ]]; then
    echo "Fehler: APP_VERSION konnte nicht aus $CONFIG_FILE gelesen werden."
    exit 1
fi

PACKAGE_ROOT="update-package"
PACKAGE_UPDATES_DIR="$PACKAGE_ROOT/updates"
VERSION_DIR="$PACKAGE_UPDATES_DIR/$VERSION"
R36S_DIR="$VERSION_DIR/r36s"
GPM2804_DIR="$VERSION_DIR/gpm2804"
LATEST_JSON="$PACKAGE_UPDATES_DIR/latest.json"
OUTPUT_DIR="updates"
ZIP_FILE="$OUTPUT_DIR/update-$VERSION.zip"

echo "Erstelle Update-ZIP fuer Version $VERSION ..."

rm -rf "$PACKAGE_ROOT"
mkdir -p "$OUTPUT_DIR"
rm -f "$ZIP_FILE"
mkdir -p "$R36S_DIR" "$GPM2804_DIR"

cp "$R36S_BINARY" "$R36S_DIR/hoerspiel_player"
cp "$GPM2804_BINARY" "$GPM2804_DIR/hoerspiel_player"

R36S_SHA256="$(sha256sum "$R36S_DIR/hoerspiel_player" | awk '{print $1}')"
GPM2804_SHA256="$(sha256sum "$GPM2804_DIR/hoerspiel_player" | awk '{print $1}')"

cat > "$LATEST_JSON" <<EOF
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

(
    cd "$PACKAGE_ROOT"
    zip -qr "../$ZIP_FILE" updates
)

rm -rf "$PACKAGE_ROOT"

echo
echo "Update-ZIP fertig: $ZIP_FILE"
echo "Version:         $VERSION"
echo "SHA256 R36S:    $R36S_SHA256"
echo "SHA256 GPM2804: $GPM2804_SHA256"
echo
echo "ZIP-Inhalt:"
unzip -l "$ZIP_FILE"
