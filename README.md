# Pitch Class Router

A lightweight utility that bridges [Scale Navigator](https://scalenavigator.com) to pitch-class-aware plugins like Fluid Pitch, Chroma, Scaler EQ, Temperance, Auto-Tune, and more.

## What It Does

Many pitch-correction and harmonic plugins expose 12 toggles—one for each pitch class (C, C#, D, etc.)—to define which notes are "in scale." Manually switching these during a session is tedious.

**Pitch Class Router** solves this by:
1. Receiving scale/chord data from Scale Navigator Dashboard
2. Converting it to MIDI CC messages
3. Sending those CCs to your DAW, where they're mapped to the plugin's pitch-class toggles

Now when you change chords or scales in Scale Navigator, your plugins update instantly.

## Compatible Plugins

Any plugin with 12 pitch-class toggles that can be MIDI-mapped:

- [Fluid Pitch](https://pitchinnovations.com/products/fluid-pitch) by Pitch Innovations
- [Chroma](https://www.xynth.audio/plugins/chroma) by Xynth Audio
- [Scaler EQ](https://scalermusic.com/products/scaler-eq/) by Plugin Boutique
- [Temperance Pro](https://www.eventideaudio.com/plug-ins/temperance-pro/) by Eventide
- [Auto-Tune Artist](https://www.antarestech.com/) by Antares
- Any other plugin with MIDI-mappable pitch-class parameters

## Requirements

- A browser with WebMIDI support (Chrome, Edge, Opera)
- A virtual MIDI port (e.g., IAC Driver on macOS, loopMIDI on Windows)
- Scale Navigator Dashboard

## Setup

### 1. Create a Virtual MIDI Port

**macOS:** Open Audio MIDI Setup → Window → Show MIDI Studio → double-click IAC Driver → create a port named "INTERSTICES" (or edit the config in the JS file to match your port name).

**Windows:** Use [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) to create a virtual port.

### 2. Map Your Plugin

1. Open your DAW and load a pitch-class plugin (e.g., Chroma, Fluid Pitch)
2. Enter MIDI Map mode (Cmd+M in Ableton Live)
3. Open `index.html` in Chrome
4. For each pitch class:
   - Click the plugin's toggle (e.g., "Note C")
   - Click the corresponding button in Pitch Class Router (e.g., "0: C")
5. Exit MIDI Map mode

### 3. Configure Scale Navigator

In Scale Navigator Dashboard:
- Set MIDI output to your virtual port (e.g., "IAC Driver INTERSTICES")
- Set output channel to **7**
- Enable pitch class output for chords, scales, or both

### 4. Play

Change chords or scales in Scale Navigator—the plugin toggles update automatically.

## How It Works

```
Scale Navigator                 Pitch Class Router              Your Plugin
(sends scales)                  (this web app)                  (receives CCs)
     │                                │                              │
     └──[Ch 7 Note On/Off]───────────→│                              │
        (note % 12 = pitch class)     │                              │
                                      │                              │
                                      └──[Ch 8 CC 20-31]────────────→│
                                         (127 = on, 0 = off)         │
```

- **Input:** MIDI channel 7, Note On/Off messages
- **Output:** MIDI channel 8, CC 20–31 (C=CC20, C#=CC21, ... B=CC31)

## Configuration

Edit the top of `pitch-class-router.js` to customize:

```javascript
const PORT_NAME_MATCH = "INTERSTICES";  // Virtual port name to match
const SCALE_IN_CHANNEL = 7;              // Channel from Scale Navigator
const OUT_CHANNEL = 8;                   // Channel to plugin
const BASE_CC = 20;                      // First CC number (C)
```

## Roadmap

This web-based version is a lightweight proof of concept. A proper JUCE-based plugin/standalone app is planned for:
- Native performance
- VST3/AU/AAX format
- Cross-platform distribution
- Integration with Scale Navigator store

## Part of the Scale Navigator Family

[Scale Navigator](https://scalenavigator.com) provides a powerful harmonic framework for exploring scales, chords, and key relationships. Pitch Class Router extends that framework to real-time plugin control.

## License

MIT
