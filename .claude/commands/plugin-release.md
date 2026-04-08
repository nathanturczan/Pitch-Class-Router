# Plugin Release (Signing & Notarization)

**For creating distributable installers.**

> **Customize:** Replace `Pitch Class Router` and version numbers with actual values

## Prerequisites

Before running this, ensure:

- [ ] `/plugin-verify` passed with no BLOCKERS
- [ ] Version number correct in CMakeLists.txt or .jucer
- [ ] All source changes committed

---

## Build Commands

### CMake Projects
```bash
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build . --target Pitch Class Router_AU Pitch Class Router_VST3 -j8
```

### Projucer Projects
```bash
# Regenerate Xcode project (if needed)
~/JUCE/Projucer.app/Contents/MacOS/Projucer --resave Pitch Class Router.jucer

# Build with Xcode
cd Builds/MacOSX
xcodebuild -project Pitch Class Router.xcodeproj \
  -scheme "Pitch Class Router - All" \
  -configuration Release \
  ARCHS="x86_64 arm64" \
  ONLY_ACTIVE_ARCH=NO
```

---

## Sign Plugins

```bash
# Sign VST3
codesign --force --sign "Developer ID Application: Nathan Turczan (9DVDAB3FWL)" \
  --timestamp --options runtime --deep \
  ~/Library/Audio/Plug-Ins/VST3/Pitch Class Router.vst3

# Sign AU
codesign --force --sign "Developer ID Application: Nathan Turczan (9DVDAB3FWL)" \
  --timestamp --options runtime --deep \
  ~/Library/Audio/Plug-Ins/Components/Pitch Class Router.component
```

---

## Create Installer Package

### Option 1: Use automated script (if exists)
```bash
./scripts/create-pkg.sh
# or
./scripts/create-installer.sh
```

### Option 2: Manual packaging
```bash
# Create staging area
mkdir -p dist/staging/{vst3,au}
cp -R ~/Library/Audio/Plug-Ins/VST3/Pitch Class Router.vst3 dist/staging/vst3/
cp -R ~/Library/Audio/Plug-Ins/Components/Pitch Class Router.component dist/staging/au/

# Build component packages
pkgbuild --root dist/staging/vst3 \
  --identifier com.scalenavigator.Pitch Class Router.vst3 \
  --version X.Y.Z \
  --install-location /Library/Audio/Plug-Ins/VST3 \
  dist/vst3.pkg

pkgbuild --root dist/staging/au \
  --identifier com.scalenavigator.Pitch Class Router.au \
  --version X.Y.Z \
  --install-location /Library/Audio/Plug-Ins/Components \
  dist/au.pkg

# Build distribution package
productbuild --synthesize \
  --package dist/vst3.pkg \
  --package dist/au.pkg \
  dist/distribution.xml

productbuild --distribution dist/distribution.xml \
  --package-path dist \
  dist/Pitch Class Router-X.Y.Z-unsigned.pkg
```

---

## Sign and Notarize Installer

```bash
# Sign installer
productsign --sign "Developer ID Installer: Nathan Turczan (9DVDAB3FWL)" \
  dist/Pitch Class Router-X.Y.Z-unsigned.pkg \
  dist/Pitch Class Router-X.Y.Z.pkg

# Notarize (wait for Apple approval)
xcrun notarytool submit dist/Pitch Class Router-X.Y.Z.pkg \
  --keychain-profile "notarytool-profile" --wait

# Staple notarization ticket
xcrun stapler staple dist/Pitch Class Router-X.Y.Z.pkg
```

---

## Verify Final Package

```bash
# Check signature
pkgutil --check-signature dist/Pitch Class Router-X.Y.Z.pkg

# Check Gatekeeper
spctl -a -vv -t install dist/Pitch Class Router-X.Y.Z.pkg
# Should say: "accepted" and "source=Notarized Developer ID"
```

---

## Post-Release Checklist

- [ ] Upload to public URL
- [ ] **Download from public URL and test** (not local copy!)
- [ ] Test on at least one non-dev machine (older macOS if possible)
- [ ] Update download links on website
- [ ] Notify users/testers

---

## Troubleshooting

**"a sealed resource is missing or invalid"**
→ Files were copied after signing. Re-run signing step.

**"Developer ID Application not found"**
→ Check available identities: `security find-identity -v -p codesigning`

**Notarization rejected**
→ Check log: `xcrun notarytool log <submission-id> --keychain-profile "notarytool-profile"`

**minos wrong (26.x instead of 11.0)**
→ CMake cache is stale. Delete build dir, rebuild with `-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0`

**Binary is x86_64 only**
→ CMake cache is stale. Delete build dir, rebuild with `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`
