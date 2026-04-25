#!/usr/bin/env bash
# Builds a Linux VST3 installer tarball: pitch-class-router-<ver>-linux-x86_64.tar.gz
# Contains the VST3 bundle, install.sh, and README.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PLUGIN_ROOT"

VERSION=$(grep -oP 'project\(PitchClassRouter VERSION \K[0-9.]+' CMakeLists.txt)
PLUGIN_DISPLAY="Pitch Class Router"
PLUGIN_SLUG="pitch-class-router"
ARCH="x86_64"
DIST_NAME="${PLUGIN_SLUG}-${VERSION}-linux-${ARCH}"
BUILD_DIR="${PLUGIN_ROOT}/build"
VST3_BUNDLE="${BUILD_DIR}/PitchClassRouter_artefacts/Release/VST3/${PLUGIN_DISPLAY}.vst3"
STANDALONE_BIN="${BUILD_DIR}/PitchClassRouter_artefacts/Release/Standalone/${PLUGIN_DISPLAY}"
STAGING="${BUILD_DIR}/installer-staging/${DIST_NAME}"
OUT_TARBALL="${BUILD_DIR}/${DIST_NAME}.tar.gz"

if [ ! -d "$VST3_BUNDLE" ]; then
    echo "VST3 bundle not found at $VST3_BUNDLE — run a Release build first."
    exit 1
fi

if ! python3 -m json.tool "$VST3_BUNDLE/Contents/Resources/moduleinfo.json" > /dev/null 2>&1; then
    echo "moduleinfo.json is invalid — build is broken."
    exit 1
fi

rm -rf "$STAGING"
mkdir -p "$STAGING"

cp -r "$VST3_BUNDLE" "$STAGING/"

# Include the standalone binary too (optional but useful for users to test outside a DAW)
if [ -f "$STANDALONE_BIN" ]; then
    cp "$STANDALONE_BIN" "$STAGING/"
fi

cat > "$STAGING/install.sh" <<'INSTALL_EOF'
#!/usr/bin/env bash
set -euo pipefail

PLUGIN="Pitch Class Router.vst3"
TARGET="$HOME/.vst3"
HERE="$(cd "$(dirname "$0")" && pwd)"

# ALSA is the runtime dep (no webkit2gtk required — pure JUCE Components UI).
if ! ldconfig -p 2>/dev/null | grep -q libasound.so; then
    echo "ERROR: libasound (ALSA) not found on this system."
    echo "  Arch / SteamOS:  sudo pacman -S alsa-lib"
    echo "  Fedora:          sudo dnf install alsa-lib"
    echo "  Debian/Ubuntu:   sudo apt install libasound2"
    echo
    echo "Install the dependency, then re-run this script."
    exit 1
fi

mkdir -p "$TARGET"
if [ -d "$TARGET/$PLUGIN" ]; then
    echo "Removing existing $TARGET/$PLUGIN"
    rm -rf "$TARGET/$PLUGIN"
fi

cp -r "$HERE/$PLUGIN" "$TARGET/"
echo "Installed VST3 to $TARGET/$PLUGIN"
echo "Restart your DAW and re-scan VST3 plugins."

# Optional: copy standalone to ~/.local/bin if it's in the package
STANDALONE="$HERE/Pitch Class Router"
if [ -f "$STANDALONE" ]; then
    mkdir -p "$HOME/.local/bin"
    cp "$STANDALONE" "$HOME/.local/bin/pitch-class-router"
    chmod +x "$HOME/.local/bin/pitch-class-router"
    echo "Installed standalone to $HOME/.local/bin/pitch-class-router"
fi
INSTALL_EOF
chmod +x "$STAGING/install.sh"

cat > "$STAGING/README.txt" <<README_EOF
Pitch Class Router v${VERSION} — Linux VST3 + Standalone
========================================================

Quick install:
    ./install.sh

This copies "${PLUGIN_DISPLAY}.vst3" into ~/.vst3/ and installs the standalone
to ~/.local/bin/pitch-class-router.

System requirements:
  - Linux x86_64
  - glibc >= 2.38
  - libasound (ALSA) — runtime dep for MIDI I/O
      Arch / SteamOS:  sudo pacman -S alsa-lib
      Fedora:          sudo dnf install alsa-lib
      Debian/Ubuntu:   sudo apt install libasound2

Manual install (skip install.sh):
    cp -r "${PLUGIN_DISPLAY}.vst3" ~/.vst3/

Then restart your DAW and re-scan VST3 plugins.

Tested in: REAPER (native Linux build) on SteamOS.

What it does:
  Bridges Scale Navigator Dashboard (or any source) to pitch-class-aware
  plugins like Fluid Pitch, Chroma, Scaler EQ, Temperance, Auto-Tune, etc.
  Receives Note On/Off on a configurable input channel and emits CC 20-31
  (per pitch class) on a configurable output channel.

MIDI port routing on Linux:
  The plugin enumerates ALSA sequencer clients via JUCE's MidiInput/Output.
  To get virtual/loopback ports between apps:
      sudo modprobe snd-virmidi   # creates VirMIDI 2-0..2-3
      qpwgraph                    # GUI patchbay for routing
  "Midi Through Port-0" is always available as a basic loopback.

Source: https://github.com/nathanturczan/Pitch-Class-Router
README_EOF

cd "${BUILD_DIR}/installer-staging"
tar -czf "$OUT_TARBALL" "$DIST_NAME"
cd "$PLUGIN_ROOT"

echo
echo "Built: $OUT_TARBALL"
ls -lh "$OUT_TARBALL"
echo
echo "Contents:"
tar -tzf "$OUT_TARBALL" | head -30
echo "..."
echo "  ($(tar -tzf "$OUT_TARBALL" | wc -l) entries total)"
