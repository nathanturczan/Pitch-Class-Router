# Pitch Class Router

A utility that bridges [Scale Navigator](https://scalenavigator.com) to pitch-class-aware plugins like Fluid Pitch, Chroma, Scaler EQ, Temperance, Auto-Tune, and more.

## What It Does

Many pitch-correction and harmonic plugins expose 12 toggles—one for each pitch class (C, C#, D, etc.)—to define which notes are "in scale." Manually switching these during a session is tedious.

**Pitch Class Router** solves this by:
1. Receiving scale/chord data from Scale Navigator Dashboard
2. Converting it to MIDI CC messages
3. Sending those CCs to your DAW, where they're mapped to the plugin's pitch-class toggles

Now when you change chords or scales in Scale Navigator, your plugins update instantly.

## Two Versions

### JUCE Plugin (Recommended)

A native VST3/AU plugin that lives inside your DAW session. Found in the `plugin/` directory.

**Features:**
- Configurable MIDI input port and channel
- Configurable MIDI output port and channel
- Configurable base CC number
- 12 mapping buttons for MIDI learn
- Visual pitch class state indicators
- Settings saved with your project

**Building:**
```bash
cd plugin
cmake -B build
cmake --build build
```

Requires CMake 3.22+ and a C++17 compiler. JUCE is fetched automatically.

### Web Version (Lightweight)

A browser-based version using WebMIDI. Just open `index.html` in Chrome.

Good for quick testing or if you prefer running outside the DAW.

## Compatible Plugins

Any plugin with 12 pitch-class toggles that can be MIDI-mapped:

- [Fluid Pitch](https://pitchinnovations.com/products/fluid-pitch) by Pitch Innovations
- [Chroma](https://www.xynth.audio/plugins/chroma) by Xynth Audio
- [Scaler EQ](https://scalermusic.com/products/scaler-eq/) by Plugin Boutique
- [Temperance Pro](https://www.eventideaudio.com/plug-ins/temperance-pro/) by Eventide
- [Auto-Tune Artist](https://www.antarestech.com/) by Antares
- Any other plugin with MIDI-mappable pitch-class parameters

## Setup

### 1. Create a Virtual MIDI Port

**macOS:** Open Audio MIDI Setup > Window > Show MIDI Studio > double-click IAC Driver > create a port (e.g., "INTERSTICES").

**Windows:** Use [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) to create a virtual port.

### 2. Load Pitch Class Router

**Plugin version:** Load Pitch Class Router on a MIDI track. Select your virtual MIDI port and channels in the plugin UI.

**Web version:** Open `index.html` in Chrome. Edit `pitch-class-router.js` to set your port name.

### 3. Map Your Target Plugin

1. Load a pitch-class plugin (e.g., Chroma, Fluid Pitch)
2. Enter MIDI Map mode (Cmd+M in Ableton Live)
3. For each pitch class:
   - Click the target plugin's toggle (e.g., "Note C")
   - Click the corresponding button in Pitch Class Router
4. Exit MIDI Map mode

### 4. Configure Scale Navigator

In Scale Navigator Dashboard:
- Set MIDI output to your virtual port
- Set output channel (default: 7)
- Enable pitch class output for chords, scales, or both

### 5. Play

Change chords or scales in Scale Navigator—the target plugin's toggles update automatically.

## How It Works

```
Scale Navigator                 Pitch Class Router              Your Plugin
(sends scales)                  (VST3/AU or web app)            (receives CCs)
     │                                │                              │
     └──[Note On/Off]────────────────→│                              │
        (note % 12 = pitch class)     │                              │
                                      │                              │
                                      └──[CC 20-31]─────────────────→│
                                         (127 = on, 0 = off)         │
```

- **Input:** Note On/Off messages (pitch class = note number mod 12)
- **Output:** CC messages (default CC 20-31 for C-B, configurable)

## Part of the Scale Navigator Family

[Scale Navigator](https://scalenavigator.com) provides a powerful harmonic framework for exploring scales, chords, and key relationships. Pitch Class Router extends that framework to real-time plugin control.

## License

MIT
