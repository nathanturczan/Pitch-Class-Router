#!/bin/bash

# verify-build.sh - Verify INSTALLED plugin (not build artifacts)
#
# IMPORTANT: Run this on the INSTALLED plugin, not the build directory.
# The installed plugin is the truth. Build artifacts can lie.
#
# Customize: Set PLUGIN_NAME and VERSION at the top

set -e

# ============================================
# CUSTOMIZE THESE FOR YOUR PLUGIN
# ============================================
PLUGIN_NAME="${PLUGIN_NAME:-PitchClassRouter}"
VERSION="${VERSION:-1.0.0}"
EXPECTED_MINOS="11.0"

# ============================================
# RELEASE IDENTITY (update each release)
# ============================================
# Version:        $VERSION
# Bundle ID:      com.scalenavigator.$PLUGIN_NAME
# Plugin Name:    $PLUGIN_NAME
# Deployment:     macOS $EXPECTED_MINOS+
# Architectures:  arm64 + x86_64

# ============================================
# Don't modify below unless necessary
# ============================================

VST3_PATH=~/Library/Audio/Plug-Ins/VST3/${PLUGIN_NAME}.vst3
AU_PATH=~/Library/Audio/Plug-Ins/Components/${PLUGIN_NAME}.component

echo "=== Plugin Build Verification ==="
echo "Plugin: $PLUGIN_NAME"
echo "Expected Version: $VERSION"
echo ""

ERRORS=0

# Check 1: VST3 exists
if [ -d "$VST3_PATH" ]; then
    echo "✓ VST3 found: $VST3_PATH"
else
    echo "✗ VST3 NOT FOUND: $VST3_PATH"
    ((ERRORS++))
fi

# Check 2: AU exists
if [ -d "$AU_PATH" ]; then
    echo "✓ AU found: $AU_PATH"
else
    echo "✗ AU NOT FOUND: $AU_PATH"
    ((ERRORS++))
fi

echo ""

# Check 3: Architecture (must be universal)
if [ -f "$VST3_PATH/Contents/MacOS/$PLUGIN_NAME" ]; then
    ARCH=$(file "$VST3_PATH/Contents/MacOS/$PLUGIN_NAME")
    if echo "$ARCH" | grep -q "universal binary" && echo "$ARCH" | grep -q "arm64" && echo "$ARCH" | grep -q "x86_64"; then
        echo "✓ Architecture: Universal (arm64 + x86_64)"
    else
        echo "✗ Architecture: NOT UNIVERSAL - BLOCKER"
        echo "  $ARCH"
        ((ERRORS++))
    fi

    # Check 4: Deployment target (minos)
    MINOS=$(otool -l "$VST3_PATH/Contents/MacOS/$PLUGIN_NAME" 2>/dev/null | grep -A1 "minos" | grep minos | awk '{print $2}' | head -1)
    if [ "$MINOS" = "$EXPECTED_MINOS" ]; then
        echo "✓ Deployment target (minos): $MINOS"
    else
        echo "✗ Deployment target (minos): $MINOS - BLOCKER (should be $EXPECTED_MINOS)"
        ((ERRORS++))
    fi
fi

echo ""

# Check 5: moduleinfo.json (VST3)
MODULEINFO="$VST3_PATH/Contents/Resources/moduleinfo.json"
if [ -f "$MODULEINFO" ]; then
    if python3 -m json.tool "$MODULEINFO" > /dev/null 2>&1; then
        echo "✓ moduleinfo.json: Valid JSON"
    else
        echo "✗ moduleinfo.json: INVALID JSON - BLOCKER"
        ((ERRORS++))
    fi
else
    echo "⚠ moduleinfo.json: Not found (may be OK for some builds)"
fi

echo ""

# Check 6: Code signature
if codesign --verify --deep --strict "$VST3_PATH" 2>/dev/null; then
    echo "✓ VST3 code signature: Valid"
else
    echo "✗ VST3 code signature: INVALID - BLOCKER"
    ((ERRORS++))
fi

if codesign --verify --deep --strict "$AU_PATH" 2>/dev/null; then
    echo "✓ AU code signature: Valid"
else
    echo "✗ AU code signature: INVALID - BLOCKER"
    ((ERRORS++))
fi

echo ""

# Check 7: Dependency sanity (otool -L)
# Any non-system path that isn't inside the bundle is a problem
if [ -f "$VST3_PATH/Contents/MacOS/$PLUGIN_NAME" ]; then
    BAD_DEPS=$(otool -L "$VST3_PATH/Contents/MacOS/$PLUGIN_NAME" 2>/dev/null | grep -v "/System\|/usr/lib\|@executable_path\|@loader_path\|:" | grep -v "^$" || true)
    if [ -z "$BAD_DEPS" ]; then
        echo "✓ Dependencies: All system or embedded"
    else
        echo "✗ Dependencies: Non-system paths found - BLOCKER"
        echo "$BAD_DEPS"
        ((ERRORS++))
    fi
fi

echo ""

# Check 8: Binary timestamp
if [ -f "$VST3_PATH/Contents/MacOS/$PLUGIN_NAME" ]; then
    TIMESTAMP=$(ls -la "$VST3_PATH/Contents/MacOS/$PLUGIN_NAME" | awk '{print $6, $7, $8}')
    echo "ℹ VST3 binary timestamp: $TIMESTAMP"
fi

echo ""

# Summary
echo "=== Summary ==="
if [ $ERRORS -eq 0 ]; then
    echo "✓ All checks passed on INSTALLED plugin."
    echo ""
    echo "NEXT: Test the exact uploaded pkg on a clean machine."
    echo "      The installed plugin is verified. The pkg might not be."
    exit 0
else
    echo "✗ $ERRORS BLOCKER(s) found."
    echo ""
    echo "STOP. Do not test. Do not release. Fix blockers first."
    exit 1
fi
