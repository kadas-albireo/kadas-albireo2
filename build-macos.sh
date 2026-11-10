#!/usr/bin/env bash
set -e

# ── Config ────────────────────────────────────────────────────────────────────
TRIPLET="arm64-osx-dynamic"
BUILD_DIR="build"
# ─────────────────────────────────────────────────────────────────────────────

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Optional: .env laden falls vorhanden (z.B. fuer NuGet-Cache-Credentials)
if [[ -f "$SCRIPT_DIR/.env" ]]; then
  set -a
  source "$SCRIPT_DIR/.env"
  set +a
fi

# Phase 1: Prerequisites
echo "==> Homebrew-Dependencies installieren..."
brew install cmake ninja automake bison flex gnu-sed pkg-config \
             autoconf-archive nasm libtool fdupes swig mono python@3.11 oras

export PATH="$(brew --prefix bison)/bin:$(brew --prefix flex)/bin:$(brew --prefix libtool)/bin:$PATH"

if [[ ! -f "$SCRIPT_DIR/share/geodata/dtm_analysis.tif" ]]; then
  echo "==> DTM Hoehendaten via oras herunterladen..."
  oras pull ghcr.io/kadas-albireo/kadas-albireo2/dtm_analysis:1.0 -o . || echo "WARN: DTM konnte nicht geladen werden"
fi

# Phase 2: cmake konfigurieren
# Lokale Paket-Hashes stimmen nicht mit CI-NuGet-Cache ueberein,
# daher wird alles von Quelle gebaut (dauert mehrere Stunden).
echo "==> cmake konfigurieren..."
cmake -S . \
      -G Ninja \
      -B "$BUILD_DIR" \
      -DCMAKE_BUILD_TYPE=Release \
      -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
      -DVCPKG_HOST_TRIPLET="$TRIPLET" \
      -DGDAL_PYTHON_BINDINGS_WITHOUT_NUMPY=YES

# Phase 3: Build
echo "==> Bauen..."
cmake --build "$BUILD_DIR" --parallel

# Phase 4: Paket bauen (.app + ZIP)
echo "==> App-Bundle zusammenstellen..."
TRIPLET_LIB="$SCRIPT_DIR/$BUILD_DIR/vcpkg_installed/$TRIPLET"
APP_DIR="$SCRIPT_DIR/$BUILD_DIR/output/bin/kadas.app"
MACDEPLOYQT="$TRIPLET_LIB/tools/Qt6/bin/macdeployqt"

mkdir -p "$APP_DIR/Contents/MacOS"
cp "$SCRIPT_DIR/$BUILD_DIR/output/bin/kadas" "$APP_DIR/Contents/MacOS/"
cp "$SCRIPT_DIR/$BUILD_DIR/output/bin/kadas.env" "$APP_DIR/Contents/MacOS/"

if [[ -f "$MACDEPLOYQT" ]]; then
  echo "==> macdeployqt ausfuehren..."
  "$MACDEPLOYQT" "$APP_DIR" \
    -libpath="$TRIPLET_LIB/lib" \
    -no-codesign \
    -verbose=1
else
  echo "WARN: macdeployqt nicht gefunden bei $MACDEPLOYQT, ueberspringe."
fi

echo "==> Zusaetzliche Bibliotheken und Ressourcen kopieren..."
# 1. Alle Dylibs und Symlinks in Frameworks kopieren
cp -a "$TRIPLET_LIB/lib"/*.dylib "$APP_DIR/Contents/Frameworks/"

# 2. QGIS Provider- und Auth-Plugins kopieren
mkdir -p "$APP_DIR/Contents/MacOS/lib/qgis/plugins"
cp "$TRIPLET_LIB/lib/qgis/plugins"/*.so "$APP_DIR/Contents/MacOS/lib/qgis/plugins/"

# 3. Qt Plugins (TLS backend, QCA crypto, SQL drivers, network, 3D renderers/loaders)
cp -R "$TRIPLET_LIB/Qt6/plugins/tls" "$APP_DIR/Contents/PlugIns/"
cp -R "$TRIPLET_LIB/Qt6/plugins/networkinformation" "$APP_DIR/Contents/PlugIns/"
cp -R "$TRIPLET_LIB/Qt6/plugins/sqldrivers" "$APP_DIR/Contents/PlugIns/"
cp -R "$TRIPLET_LIB/Qt6/plugins/renderers" "$APP_DIR/Contents/PlugIns/"
cp -R "$TRIPLET_LIB/Qt6/plugins/sceneparsers" "$APP_DIR/Contents/PlugIns/"
cp -R "$TRIPLET_LIB/Qt6/plugins/geometryloaders" "$APP_DIR/Contents/PlugIns/"
mkdir -p "$APP_DIR/Contents/PlugIns/crypto"
cp "$TRIPLET_LIB/bin/Qca/crypto"/* "$APP_DIR/Contents/PlugIns/crypto/" || true

for f in "$APP_DIR/Contents/PlugIns"/*/*.dylib; do
  install_name_tool -add_rpath @loader_path/../../Frameworks "$f" 2>/dev/null || true
done

# 4. Kadas Share-Ressourcen (settings, templates, print templates)
mkdir -p "$APP_DIR/Contents/share/kadas"
cp -R share/* "$APP_DIR/Contents/share/kadas/"

# 5. Kadas Python-Bindings und Python 3 Runtime kopieren
mkdir -p "$APP_DIR/Contents/share/kadas/python"
cp -R "$BUILD_DIR/output/python/kadas" "$APP_DIR/Contents/share/kadas/python/"

echo "==> Python Runtime und Packages bündeln..."
mkdir -p "$APP_DIR/Contents/lib"
if [[ ! -d "$APP_DIR/Contents/lib/python3.12" ]]; then
  cp -a "$TRIPLET_LIB/lib/python3.12" "$APP_DIR/Contents/lib/"
fi

# 6. Share-Symlink fuer QGIS-Ressourcen im MacOS-Ordner erstellen
rm -rf "$APP_DIR/Contents/share/share"
rm -f "$APP_DIR/Contents/MacOS/share"
ln -s ../share "$APP_DIR/Contents/MacOS/share"

# 7. App Icon und Info.plist einbinden
mkdir -p "$APP_DIR/Contents/Resources"
if [[ -f "$SCRIPT_DIR/kadas/resources/kadas.icns" ]]; then
  cp "$SCRIPT_DIR/kadas/resources/kadas.icns" "$APP_DIR/Contents/Resources/"
fi

cat << 'EOF' > "$APP_DIR/Contents/Info.plist"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleDisplayName</key>
	<string>KADAS Albireo</string>
	<key>CFBundleExecutable</key>
	<string>kadas</string>
	<key>CFBundleIconFile</key>
	<string>kadas.icns</string>
	<key>CFBundleIdentifier</key>
	<string>ch.sourcepole.kadas</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>KADAS Albireo</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>2.0.0</string>
	<key>CFBundleVersion</key>
	<string>2.0.0</string>
	<key>NSHighResolutionCapable</key>
	<true/>
	<key>NSSupportsAutomaticGraphicsSwitching</key>
	<true/>
	<key>NSHumanReadableCopyright</key>
	<string>Copyright (C) KADAS Albireo Team</string>
</dict>
</plist>
EOF

# 8. Rpaths bereinigen fuer volle Portabilitaet
install_name_tool -add_rpath @executable_path/../Frameworks "$APP_DIR/Contents/MacOS/kadas" 2>/dev/null || true
install_name_tool -delete_rpath "$SCRIPT_DIR/$BUILD_DIR/vcpkg_installed/$TRIPLET/lib" "$APP_DIR/Contents/MacOS/kadas" 2>/dev/null || true
install_name_tool -delete_rpath "$SCRIPT_DIR/$BUILD_DIR/output/lib" "$APP_DIR/Contents/MacOS/kadas" 2>/dev/null || true

# 9. Saubere Ad-hoc Codesignierung auf dem fertigen Bundle anwenden
echo "==> Ad-hoc Codesigning durchfuehren..."
codesign --force --deep --sign - "$APP_DIR" 2>/dev/null || true

echo "==> ZIP erstellen..."
rm -f "$SCRIPT_DIR/$BUILD_DIR/kadas-macos-arm64.zip"
(cd "$SCRIPT_DIR/$BUILD_DIR" && zip -ryq kadas-macos-arm64.zip "output/bin/kadas.app")

echo "==> DMG Installer erstellen..."
DMG_STAGING="/tmp/kadas_dmg_staging_$$"
rm -rf "$DMG_STAGING" "$SCRIPT_DIR/$BUILD_DIR/kadas-macos-arm64.dmg"
mkdir -p "$DMG_STAGING"
ln -s /Applications "$DMG_STAGING/Applications"
cp -a "$APP_DIR" "$DMG_STAGING/"

hdiutil create \
  -volname "KADAS Albireo" \
  -srcfolder "$DMG_STAGING" \
  -ov \
  -format UDZO \
  "$SCRIPT_DIR/$BUILD_DIR/kadas-macos-arm64.dmg" >/dev/null

rm -rf "$DMG_STAGING"

echo ""
echo "=========================================================="
echo "Fertig!"
echo "App Bundle:   $APP_DIR"
echo "ZIP-Archiv:   $BUILD_DIR/kadas-macos-arm64.zip"
if [[ -f "$SCRIPT_DIR/$BUILD_DIR/kadas-macos-arm64.dmg" ]]; then
  echo "DMG-Image:    $BUILD_DIR/kadas-macos-arm64.dmg"
fi
echo "=========================================================="
echo "Portabel: Kann per USB-Stick oder Google Drive auf jeden"
echo "Apple Silicon Mac kopiert und direkt gestartet werden."
