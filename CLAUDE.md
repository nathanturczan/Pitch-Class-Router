# Notes for Claude Working on Pitch Class Router

## Project Overview

**Pitch Class Router** is a JUCE audio plugin (AU/VST3/Standalone) that bridges Scale Navigator Dashboard to pitch-class-aware plugins like Fluid Pitch, Chroma, Scaler EQ, Temperance, Auto-Tune, etc.

It receives Note On/Off messages from Scale Navigator (indicating which pitch classes are in the current scale/chord) and converts them to MIDI CC messages that can be mapped to plugin parameters.

**Part of the Scale Navigator family of software.**

## Development Machine

- **MacBook Pro 16-inch, 2021**
- **Chip:** Apple M1 Max (ARM64)
- **Memory:** 64 GB
- **macOS:** Tahoe 26.2

**Important:** Must build universal binaries (arm64 + x86_64) for compatibility.

## Repository Structure

```
pitch-class-router/
├── README.md                    # Project documentation
├── index.html                   # Web version (lightweight/legacy)
├── pitch-class-router.js        # Web version logic
└── plugin/                      # JUCE plugin (recommended)
    ├── CMakeLists.txt           # Build configuration
    ├── .gitignore               # Excludes build/
    └── Source/
        ├── PluginProcessor.h    # MIDI processing, external MIDI port access
        ├── PluginProcessor.cpp
        ├── PluginEditor.h       # UI: dropdowns, 12 mapping buttons
        └── PluginEditor.cpp
```

## Build System

This project uses **CMake** (not Projucer). JUCE 8.0.4 is fetched automatically via FetchContent.

```bash
# Configure and build (from plugin/ directory)
cd plugin
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j8

# Or just rebuild after changes:
cd plugin/build && make -j8
```

**Build outputs:**
- `plugin/build/PitchClassRouter_artefacts/Release/AU/Pitch Class Router.component`
- `plugin/build/PitchClassRouter_artefacts/Release/VST3/Pitch Class Router.vst3`
- `plugin/build/PitchClassRouter_artefacts/Release/Standalone/Pitch Class Router.app`

**Post-build auto-install:** Plugins are automatically copied to:
- AU: `~/Library/Audio/Plug-Ins/Components/`
- VST3: `~/Library/Audio/Plug-Ins/VST3/`

## Code Signing

After building, plugins need ad-hoc signing for macOS to load them:

```bash
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/Components/Pitch\ Class\ Router.component
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/VST3/Pitch\ Class\ Router.vst3
```

## Plugin Validation

```bash
# Validate AU
auval -v aumi Pcrt Snav

# Check if AU is recognized
auval -a | grep -i pitch
```

## How It Works

```
Scale Navigator Dashboard          Pitch Class Router              Target Plugin
(sends scale/chord info)           (this plugin)                   (Chroma, Fluid Pitch, etc.)
        │                                │                              │
        └──[Note On/Off Ch 7]───────────→│                              │
           (note % 12 = pitch class)     │                              │
                                         │                              │
                                         └──[CC 20-31 Ch 8]────────────→│
                                            (127 = on, 0 = off)         │
```

### Plugin Features

- **MIDI Input dropdown** - Select external MIDI port (e.g., "IAC Driver INTERSTICES")
- **Input Channel selector** - Default: channel 7
- **MIDI Output dropdown** - Select output port
- **Output Channel selector** - Default: channel 8
- **Base CC selector** - Default: CC 20 (so C=CC20, C#=CC21, ... B=CC31)
- **12 Mapping Buttons** - Click to send CC for MIDI learn workflow
- **State Indicators** - Green when pitch class is active

### Mapping Workflow

1. Load Pitch Class Router on a MIDI track
2. Configure MIDI input/output ports and channels
3. Load target plugin (e.g., Chroma)
4. Enter DAW's MIDI Map mode (Cmd+M in Ableton)
5. Click target plugin's "Note C" toggle
6. Click "C" button in Pitch Class Router
7. Repeat for all 12 pitch classes
8. Exit MIDI Map mode

## Key Implementation Details

### PluginProcessor

- Inherits from `juce::AudioProcessor` and `juce::MidiInputCallback`
- Uses `juce::MidiInput::openDevice()` and `juce::MidiOutput::openDevice()` for external MIDI port access
- Maintains `std::array<bool, 12> pitchClassState` for state tracking
- Only sends CC when state changes (optimization)
- Settings saved via `getStateInformation()` / `setStateInformation()`

### PluginEditor

- Dropdown ComboBoxes for port/channel selection
- 12 TextButtons for mapping helper
- Timer-based UI updates (10Hz) for state indicators
- Dark theme (#1a1a1a background)

## Plugin Identifiers

- **Bundle ID:** `com.scalenavigator.pitchclassrouter`
- **Manufacturer Code:** `Snav`
- **Plugin Code:** `Pcrt`
- **AU Type:** `aumi` (Audio Unit MIDI Processor)

## Compatible Target Plugins

- [Fluid Pitch](https://pitchinnovations.com/products/fluid-pitch) by Pitch Innovations
- [Chroma](https://www.xynth.audio/plugins/chroma) by Xynth Audio
- [Scaler EQ](https://scalermusic.com/products/scaler-eq/) by Plugin Boutique
- [Temperance Pro](https://www.eventideaudio.com/plug-ins/temperance-pro/) by Eventide
- [Auto-Tune Artist](https://www.antarestech.com/) by Antares
- Any plugin with 12 MIDI-mappable pitch class toggles

## Related Projects

- **Scale Navigator Dashboard** - Main app that sends scale/chord data
- **Scale Awareness Bridge** - M4L device for Ableton's Scale Awareness feature (separate repo)
- **PitchSnap** - Another Scale Navigator plugin

## Current Status / Known Issues

1. **Universal binary required** - CMakeLists.txt has `set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")` but may need clean rebuild
2. **Code signing** - Requires manual ad-hoc signing after build
3. **DAW rescan** - After installing, DAW needs plugin rescan to detect

## Troubleshooting

**Plugin not appearing in DAW:**
1. Check plugins are in `~/Library/Audio/Plug-Ins/`
2. Run `auval -a | grep -i pitch` to verify AU is recognized
3. If "Cannot open component" error, re-sign with codesign
4. Rescan plugins in DAW preferences or restart DAW

**Wrong architecture:**
```bash
# Check architecture
file ~/Library/Audio/Plug-Ins/VST3/Pitch\ Class\ Router.vst3/Contents/MacOS/Pitch\ Class\ Router
# Should show: Mach-O universal binary with 2 architectures: [x86_64:Mach-O 64-bit bundle x86_64] [arm64:Mach-O 64-bit bundle arm64]
```

## Windows Porting Notes (2026-02-10)

### Changes Made
- **CMakeLists.txt**: Wrapped `CMAKE_OSX_ARCHITECTURES` and macOS post-build install commands (AU + VST3 copy to `~/Library/`) in `if(APPLE)` guards. Without this, CMake fails on Windows because the `PitchClassRouter_AU` target doesn't exist.
- No source code changes were required — the plugin is pure C++ JUCE with no platform-specific code.

### macOS Impact
- None. The `if(APPLE)` guards are no-ops on macOS — the exact same code paths execute as before. Verify by rebuilding on Mac if concerned.

### Windows Build
```bash
cd plugin
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
# Output: plugin/build/PitchClassRouter_artefacts/Release/VST3/Pitch Class Router.vst3
# Standalone: plugin/build/PitchClassRouter_artefacts/Release/Standalone/Pitch Class Router.exe
```

### Windows VST3 Install
Copy VST3 to `C:\Program Files\Common Files\VST3\` (requires admin).

## GitHub

- **Repo:** https://github.com/nathanturczan/Pitch-Class-Router
- **License:** MIT
