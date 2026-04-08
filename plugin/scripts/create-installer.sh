#!/bin/bash

# Pitch Class Router - macOS Installer Creator
# Creates a signed and notarized DMG with AU and VST3 plugins

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$PROJECT_ROOT/dist"
VERSION="1.0.2"
DMG_NAME="PitchClassRouter-${VERSION}-macOS"
PLUGIN_NAME="Pitch Class Router"

# Signing identity
SIGNING_IDENTITY="Developer ID Application: Nathan Turczan (9DVDAB3FWL)"
KEYCHAIN_PROFILE="notarytool-profile"

echo "=== Pitch Class Router Installer Creator ==="
echo ""

# Clean and create dist directory
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR/$PLUGIN_NAME"

# Build in Release mode if not already built
echo "Checking build..."
if [ ! -d "$BUILD_DIR/PitchClassRouter_artefacts/Release" ]; then
    echo "Building Release configuration..."
    cd "$PROJECT_ROOT"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release -j$(sysctl -n hw.ncpu)
fi

echo ""
echo "Copying plugin artifacts..."

# Copy AU plugin
AU_SRC="$BUILD_DIR/PitchClassRouter_artefacts/Release/AU/$PLUGIN_NAME.component"
if [ -d "$AU_SRC" ]; then
    cp -R "$AU_SRC" "$DIST_DIR/$PLUGIN_NAME/$PLUGIN_NAME.component"
    echo "  AU Component copied"
else
    echo "  AU Component not found at: $AU_SRC"
    exit 1
fi

# Copy VST3 plugin
VST3_SRC="$BUILD_DIR/PitchClassRouter_artefacts/Release/VST3/$PLUGIN_NAME.vst3"
if [ -d "$VST3_SRC" ]; then
    cp -R "$VST3_SRC" "$DIST_DIR/$PLUGIN_NAME/$PLUGIN_NAME.vst3"
    echo "  VST3 Plugin copied"
else
    echo "  VST3 Plugin not found at: $VST3_SRC"
    exit 1
fi

# Copy Standalone app
APP_SRC="$BUILD_DIR/PitchClassRouter_artefacts/Release/Standalone/$PLUGIN_NAME.app"
if [ -d "$APP_SRC" ]; then
    cp -R "$APP_SRC" "$DIST_DIR/$PLUGIN_NAME/$PLUGIN_NAME.app"
    echo "  Standalone App copied"
else
    echo "  Standalone App not found at: $APP_SRC"
    exit 1
fi

# Code sign all plugins
echo ""
echo "Code signing plugins..."

# Sign AU Component
codesign --force --deep --options runtime \
    --sign "$SIGNING_IDENTITY" \
    "$DIST_DIR/$PLUGIN_NAME/$PLUGIN_NAME.component"
echo "  AU Component signed"

# Sign VST3 Plugin
codesign --force --deep --options runtime \
    --sign "$SIGNING_IDENTITY" \
    "$DIST_DIR/$PLUGIN_NAME/$PLUGIN_NAME.vst3"
echo "  VST3 Plugin signed"

# Sign Standalone App
codesign --force --deep --options runtime \
    --sign "$SIGNING_IDENTITY" \
    "$DIST_DIR/$PLUGIN_NAME/$PLUGIN_NAME.app"
echo "  Standalone App signed"

# Verify signatures
echo ""
echo "Verifying signatures..."
codesign --verify --verbose "$DIST_DIR/$PLUGIN_NAME/$PLUGIN_NAME.component" && echo "  AU verified"
codesign --verify --verbose "$DIST_DIR/$PLUGIN_NAME/$PLUGIN_NAME.vst3" && echo "  VST3 verified"
codesign --verify --verbose "$DIST_DIR/$PLUGIN_NAME/$PLUGIN_NAME.app" && echo "  Standalone verified"

# Create installation instructions
cat > "$DIST_DIR/$PLUGIN_NAME/README.txt" << 'EOF'
Pitch Class Router - Installation Guide
========================================

This package contains:
- Pitch Class Router.component (Audio Unit for Logic, GarageBand, etc.)
- Pitch Class Router.vst3 (VST3 for Ableton Live, etc.)
- Pitch Class Router.app (Standalone application)

INSTALLATION
------------

For AU (Logic Pro, GarageBand):
  Copy "Pitch Class Router.component" to:
  ~/Library/Audio/Plug-Ins/Components/

For VST3 (Ableton Live, etc.):
  Copy "Pitch Class Router.vst3" to:
  ~/Library/Audio/Plug-Ins/VST3/

The Standalone app can be placed in /Applications or run from anywhere.

To access these folders:
1. Open Finder
2. Press Cmd+Shift+G
3. Paste the path above

After installation, restart your DAW and rescan plugins if needed.

WHAT IS PITCH CLASS ROUTER?
---------------------------
Pitch Class Router bridges Scale Navigator Dashboard to pitch-class-aware
plugins like Fluid Pitch, Chroma, Scaler EQ, Temperance, and Auto-Tune.

It receives scale/chord information from Dashboard and converts it to
MIDI CC messages that can be mapped to plugin parameters.

REQUIREMENTS
------------
- macOS 10.15 (Catalina) or later
- Scale Navigator Dashboard for sending scale data

SUPPORT
-------
Website: https://scalenavigator.com
Report issues at: https://github.com/nathanturczan/Pitch-Class-Router

EOF

echo "  README created"

# Create DMG
echo ""
echo "Creating DMG..."

rm -f "$DIST_DIR/$DMG_NAME.dmg"

hdiutil create -volname "Pitch Class Router $VERSION" \
    -srcfolder "$DIST_DIR/$PLUGIN_NAME" \
    -ov -format UDZO \
    "$DIST_DIR/$DMG_NAME.dmg"

# Sign the DMG
echo ""
echo "Signing DMG..."
codesign --force --sign "$SIGNING_IDENTITY" "$DIST_DIR/$DMG_NAME.dmg"
echo "  DMG signed"

# Notarize the DMG
echo ""
echo "Submitting for notarization (this may take a few minutes)..."
xcrun notarytool submit "$DIST_DIR/$DMG_NAME.dmg" \
    --keychain-profile "$KEYCHAIN_PROFILE" \
    --wait

# Staple the notarization ticket
echo ""
echo "Stapling notarization ticket..."
xcrun stapler staple "$DIST_DIR/$DMG_NAME.dmg"
echo "  Notarization ticket stapled"

# Verify notarization
echo ""
echo "Verifying notarization..."
spctl --assess --type open --context context:primary-signature --verbose "$DIST_DIR/$DMG_NAME.dmg"

echo ""
echo "=== Done! ==="
echo ""
echo "Signed and notarized installer created at:"
echo "  $DIST_DIR/$DMG_NAME.dmg"
echo ""
echo "Contents:"
ls -la "$DIST_DIR/$PLUGIN_NAME/"
echo ""
echo "DMG size: $(du -h "$DIST_DIR/$DMG_NAME.dmg" | cut -f1)"
echo ""
echo "This DMG is ready to distribute - no security warnings for users!"
