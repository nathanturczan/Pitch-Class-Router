# Plugin Build Verification

**MANDATORY after every build. No "ready to test" without this output.**

Run ALL checks below. Paste ALL output. Do not summarize or skip.

> **Customize:** Replace `Pitch Class Router` with actual plugin name (e.g., `Tonalign`, `SNaPS`, `Bender`)

## HARD BLOCKERS (Release cannot proceed if any fail)

- Binary minos matches target (11.0), not dev machine's OS
- Binary is universal (arm64 + x86_64)
- moduleinfo.json is valid JSON (VST3 only)
- Code signature is valid

---

## 1. Check for duplicate installs

```bash
echo "=== USER VST3 ===" && ls -la ~/Library/Audio/Plug-Ins/VST3/ 2>/dev/null | grep -i Pitch Class Router
echo "=== SYSTEM VST3 ===" && ls -la /Library/Audio/Plug-Ins/VST3/ 2>/dev/null | grep -i Pitch Class Router
echo "=== USER AU ===" && ls -la ~/Library/Audio/Plug-Ins/Components/ 2>/dev/null | grep -i Pitch Class Router
echo "=== SYSTEM AU ===" && ls -la /Library/Audio/Plug-Ins/Components/ 2>/dev/null | grep -i Pitch Class Router
```

**FAIL if:** Duplicates exist in both user AND system locations. Delete the stale one.

## 2. Check deployment target (minos) - BLOCKER

```bash
echo "=== DEPLOYMENT TARGET ===" && otool -l ~/Library/Audio/Plug-Ins/VST3/Pitch Class Router.vst3/Contents/MacOS/Pitch Class Router | grep -A3 "LC_BUILD_VERSION" | head -8
```

**REQUIRED:** `minos 11.0`
**BLOCKER if:** minos shows your dev OS (e.g., 26.2). Plugin won't load on older macOS.

## 3. Check architecture - BLOCKER

```bash
echo "=== VST3 ARCH ===" && file ~/Library/Audio/Plug-Ins/VST3/Pitch Class Router.vst3/Contents/MacOS/Pitch Class Router
echo "=== AU ARCH ===" && file ~/Library/Audio/Plug-Ins/Components/Pitch Class Router.component/Contents/MacOS/Pitch Class Router
```

**REQUIRED:** `Mach-O universal binary with 2 architectures`
**BLOCKER if:** Single architecture (x86_64 only or arm64 only)

## 4. Check moduleinfo.json - BLOCKER (VST3 only)

```bash
echo "=== MODULEINFO JSON ===" && python3 -m json.tool ~/Library/Audio/Plug-Ins/VST3/Pitch Class Router.vst3/Contents/Resources/moduleinfo.json > /dev/null && echo "JSON: VALID" || echo "JSON: INVALID - BLOCKER"
```

**BLOCKER if:** JSON is invalid (trailing commas cause "not a plugin" on some hosts)

## 5. Check code signature - BLOCKER

```bash
echo "=== VST3 SIGNATURE ===" && codesign --verify --deep --strict ~/Library/Audio/Plug-Ins/VST3/Pitch Class Router.vst3 2>&1 && echo "VST3: VALID" || echo "VST3: INVALID - BLOCKER"
echo "=== AU SIGNATURE ===" && codesign --verify --deep --strict ~/Library/Audio/Plug-Ins/Components/Pitch Class Router.component 2>&1 && echo "AU: VALID" || echo "AU: INVALID - BLOCKER"
```

**BLOCKER if:** Signature errors (causes "sealed resource missing" on load)

## 6. Check timestamps

```bash
echo "=== TIMESTAMPS ===" && ls -la ~/Library/Audio/Plug-Ins/VST3/Pitch Class Router.vst3/Contents/MacOS/Pitch Class Router && ls -la ~/Library/Audio/Plug-Ins/Components/Pitch Class Router.component/Contents/MacOS/Pitch Class Router
```

**FAIL if:** Timestamp is older than your source file changes.

## 7. Check version string in binary

```bash
echo "=== VERSION STRINGS ===" && strings ~/Library/Audio/Plug-Ins/VST3/Pitch Class Router.vst3/Contents/MacOS/Pitch Class Router | grep -iE "Pitch Class Router|[0-9]+\.[0-9]+\.[0-9]+" | head -5
```

**FAIL if:** Version string doesn't match expected version.

## 8. AU Validation (quick check)

```bash
echo "=== AU VALIDATION ===" && auval -a 2>&1 | grep -i "Pitch Class Router"
auval -v aumi Pcrt Snav
```

---

## VERDICT

After running all checks, state one of:

- **PASS** - All checks passed, no blockers. Ready to test in DAW.
- **BLOCKER: [reason]** - Hard blocker found. MUST fix before testing or release.
- **FAIL: [reason]** - Soft failure. Should fix but not a blocker.

**NEVER say "ready to test" without showing this full output first.**
