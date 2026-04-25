# Plugin Build Verification

**MANDATORY after every build. No "ready to test" without this output.**

Run ALL checks for the platform you built on. Paste ALL output. Do not summarize or skip.

Detect platform first:

```bash
uname -s
# Darwin -> run macOS section
# Linux  -> run Linux section
```

---

# macOS

## HARD BLOCKERS (Release cannot proceed if any fail)

- Binary minos matches target (11.0), not dev machine's OS
- Binary is universal (arm64 + x86_64)
- moduleinfo.json is valid JSON (VST3 only)
- Code signature is valid

## 1. Check for duplicate installs

```bash
echo "=== USER VST3 ===" && ls -la ~/Library/Audio/Plug-Ins/VST3/ 2>/dev/null | grep -i "Pitch Class Router"
echo "=== SYSTEM VST3 ===" && ls -la /Library/Audio/Plug-Ins/VST3/ 2>/dev/null | grep -i "Pitch Class Router"
echo "=== USER AU ===" && ls -la ~/Library/Audio/Plug-Ins/Components/ 2>/dev/null | grep -i "Pitch Class Router"
echo "=== SYSTEM AU ===" && ls -la /Library/Audio/Plug-Ins/Components/ 2>/dev/null | grep -i "Pitch Class Router"
```

**FAIL if:** Duplicates exist in both user AND system locations. Delete the stale one.

## 2. Check deployment target (minos) - BLOCKER

```bash
otool -l ~/Library/Audio/Plug-Ins/VST3/"Pitch Class Router.vst3"/Contents/MacOS/"Pitch Class Router" | grep -A3 "LC_BUILD_VERSION" | head -8
```

**REQUIRED:** `minos 11.0`. Plugin won't load on older macOS otherwise.

## 3. Check architecture - BLOCKER

```bash
file ~/Library/Audio/Plug-Ins/VST3/"Pitch Class Router.vst3"/Contents/MacOS/"Pitch Class Router"
file ~/Library/Audio/Plug-Ins/Components/"Pitch Class Router.component"/Contents/MacOS/"Pitch Class Router"
```

**REQUIRED:** `Mach-O universal binary with 2 architectures`.

## 4. Check moduleinfo.json - BLOCKER (VST3)

```bash
python3 -m json.tool ~/Library/Audio/Plug-Ins/VST3/"Pitch Class Router.vst3"/Contents/Resources/moduleinfo.json > /dev/null && echo "JSON: VALID" || echo "JSON: INVALID - BLOCKER"
```

**BLOCKER if:** Invalid (trailing commas -> "not a plugin" on some hosts). Fix: `python3 plugin/scripts/fix-moduleinfo.py <path>`.

## 5. Check code signature - BLOCKER

```bash
codesign --verify --deep --strict ~/Library/Audio/Plug-Ins/VST3/"Pitch Class Router.vst3" 2>&1 && echo "VST3: VALID" || echo "VST3: INVALID - BLOCKER"
codesign --verify --deep --strict ~/Library/Audio/Plug-Ins/Components/"Pitch Class Router.component" 2>&1 && echo "AU: VALID" || echo "AU: INVALID - BLOCKER"
```

## 6. Check timestamps

```bash
ls -la ~/Library/Audio/Plug-Ins/VST3/"Pitch Class Router.vst3"/Contents/MacOS/"Pitch Class Router"
ls -la ~/Library/Audio/Plug-Ins/Components/"Pitch Class Router.component"/Contents/MacOS/"Pitch Class Router"
```

## 7. Version string in binary

```bash
strings ~/Library/Audio/Plug-Ins/VST3/"Pitch Class Router.vst3"/Contents/MacOS/"Pitch Class Router" | grep -iE "Pitch Class Router|[0-9]+\.[0-9]+\.[0-9]+" | head -5
```

## 8. AU validation

```bash
auval -a 2>&1 | grep -i "Pitch Class Router"
auval -v aumi Pcrt Snav
```

---

# Linux

## HARD BLOCKERS

- ELF binary, x86_64, no missing libs (`ldd` clean)
- Max GLIBC symbol <= 2.38 (portability)
- `moduleinfo.json` is valid JSON in BOTH build dir AND `~/.vst3/...` (JUCE auto-install runs before custom POST_BUILD)

## 1. Install layout

```bash
VST3="$HOME/.vst3/Pitch Class Router.vst3"
echo "=== INSTALL TREE ==="
find "$VST3" -maxdepth 5 -printf "%p %s\n"
```

**FAIL if:** No `Contents/x86_64-linux/Pitch Class Router.so` or no `Contents/Resources/`.

## 2. Architecture & file type - BLOCKER

```bash
SO="$HOME/.vst3/Pitch Class Router.vst3/Contents/x86_64-linux/Pitch Class Router.so"
file "$SO"
```

**REQUIRED:** `ELF 64-bit LSB shared object, x86-64`.

## 3. ldd - no missing libs - BLOCKER

```bash
ldd "$HOME/.vst3/Pitch Class Router.vst3/Contents/x86_64-linux/Pitch Class Router.so" 2>&1 | grep "not found" || echo "(none - OK)"
```

**BLOCKER if:** Anything `=> not found`. Likely culprit: `libasound.so.2` (ALSA).

## 4. moduleinfo.json - BLOCKER (BOTH locations)

```bash
VST3_BUILD="/home/deck/github/Pitch-Class-Router/plugin/build/PitchClassRouter_artefacts/Release/VST3/Pitch Class Router.vst3"
VST3_INSTALL="$HOME/.vst3/Pitch Class Router.vst3"
python3 -m json.tool "$VST3_BUILD/Contents/Resources/moduleinfo.json" > /dev/null && echo "BUILD JSON: VALID" || echo "BUILD JSON: INVALID - BLOCKER"
python3 -m json.tool "$VST3_INSTALL/Contents/Resources/moduleinfo.json" > /dev/null && echo "INSTALL JSON: VALID" || echo "INSTALL JSON: INVALID - BLOCKER"
```

**BLOCKER if:** Invalid (trailing commas). Fix: `python3 plugin/scripts/fix-moduleinfo.py <path>`. Both must pass.

## 5. Max GLIBC symbol used - portability

```bash
objdump -T "$HOME/.vst3/Pitch Class Router.vst3/Contents/x86_64-linux/Pitch Class Router.so" 2>/dev/null | awk '/GLIBC_/ {match($0, /GLIBC_[0-9.]+/); print substr($0,RSTART,RLENGTH)}' | sort -V -u | tail -3
```

**TARGET:** `GLIBC_2.38` or lower -> runs on Ubuntu 22.04+, Debian 12+, SteamOS, Fedora 38+.

## 6. Version string in binary

```bash
strings "$HOME/.vst3/Pitch Class Router.vst3/Contents/x86_64-linux/Pitch Class Router.so" | grep -E "^1\.[0-9]+\.[0-9]+$" | sort -u | head -3
```

**FAIL if:** Doesn't match expected version (currently `1.0.2`).

## 7. Standalone present

```bash
ls -la "/home/deck/github/Pitch-Class-Router/plugin/build/PitchClassRouter_artefacts/Release/Standalone/Pitch Class Router"
```

## 8. ALSA MIDI ports available (sanity)

```bash
aconnect -l
```

The plugin's MIDI Input/Output dropdowns enumerate ALSA sequencer clients. `Midi Through Port-0` is the always-present kernel loopback. For more virtual ports: `sudo modprobe snd-virmidi` + `qpwgraph` for routing.

## 9. REAPER scan record

```bash
grep -i "pitch class router" ~/.config/REAPER/reaper-vstplugins64.ini 2>/dev/null | head -3
```

**FAIL if:** No entry. Re-scan in REAPER -> Preferences -> VST.

## 10. Smoke test - Standalone launch

```bash
DISPLAY=:0 "/home/deck/github/Pitch-Class-Router/plugin/build/PitchClassRouter_artefacts/Release/Standalone/Pitch Class Router" &
PID=$!
sleep 3
pgrep -af "Pitch Class Router" && echo "Process: ALIVE" || echo "Process: DEAD - BLOCKER"
kill $PID 2>/dev/null
```

---

## VERDICT

After running all checks for the relevant platform, state one of:

- **PASS** - All checks passed, no blockers. Ready to test in DAW.
- **BLOCKER: [reason]** - Hard blocker found. MUST fix before testing or release.
- **FAIL: [reason]** - Soft failure. Should fix but not a blocker.

**NEVER say "ready to test" without showing this full output first.**
