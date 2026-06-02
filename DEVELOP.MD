# ALVIS — Developer Guide

## Build System

- **`EXTDEV`** must be set in your environment — points to your SDK root (e.g. `C:\Users\Karthik\EXTDEV`). `BUILD.BAT` exits with an error if it's undefined.
- **`MINGDIR`** is derived as `%EXTDEV%\MinGW32` (the 32-bit MinGW compiler directory).
- **Compiler:** `%MINGDIR%\bin\gcc`
- **Script:** `BUILD.BAT` (run from repo root)
- **Output:** `DIST\vis_alvis.dll`
- **Defines:** `-DBUILDING_DLL=1` (add `-DALLEGRO_STATICLINK` for static build)
- **Optimisation:** `-O1 -Wall` (release); `-O0 -g -DDEBUG=1` (debug)
- **Allegro:** Dynamic by default (`-lalleg`, requires `alleg42.dll` at runtime); set `STATIC=1` for static link (`-lalleg_s`)

```
BUILD.BAT                  → DIST\vis_alvis.dll  (dynamic)
set STATIC=1 && BUILD.BAT  → DIST\vis_alvis.dll  (static)
```

### External Libraries

| Library     | Version   | Notes                          |
|-------------|-----------|--------------------------------|
| **Allegro** | 4.x       | Dynamic (`-lalleg`) or static (`-lalleg_s`) |
| **Winamp SDK** | 5.x   | `vis.h`, `wa_ipc.h` (included) |

### Library Layout (under `%EXTDEV%`)

```
%EXTDEV%\MinGW32\                         → Compiler (bin\gcc)
%EXTDEV%\Allegro-MSVC8-MinGW-BCC\         → Allegro (Include\, Lib\MinGW\x86\, Bin\x86\alleg42.dll)
```

## Source Layout

```
SRC\
|-- ALVIS.C         — Entry point, config/init/quit, WndProc, Allegro helpers
|-- COMMON.C        — Winamp IPC, song title parsing, OSD text formatting
|-- BEAT.C / BEAT.H — Beat detection (ported from MilkDrop / Vis_MegaSDK)
|-- DECL.H         — Shared declarations, enums
|-- VIS_MOD.H      — Module registration system (VisModuleDef, visModules[])
|-- VIS_WAVE.C     — WaveForm Analyser
|-- VIS_SPEC.C     — Spectrum Analyser
|-- VIS_VU.C       — VU Analyser
|-- VIS_GRND.C     — The Grind
|-- VIS_BOOM.C     — Boom Box - Explosion (self-contained: init, quit, input, config)
|-- FFT.C / FFT.H  — FFT implementation
|-- VIS.H          — Winamp Vis SDK header
|-- WA_IPC.H       — Winamp IPC protocol header
```

## Architecture

ALVIS exposes five `winampVisModule` structs through a single `winampVisGetHeader()` entry point. Each module self-registers via a `__attribute__((constructor))` function that calls `registerModule()` with its `VisModuleDef`, which is built into a `winampVisModule` at load time. Modules share `config()`, `init()`, and `quit()` but have their own render function. The `getModule()` function dispatches by index from the dynamically-built array.

### Module Definition Fields

| Field | Meaning |
|-------|---------|
| `latencyMs` | Audio latency — lower = more responsive but more CPU |
| `delayMs` | Frame period — 25 ms = ~40 fps, 5 ms = ~200 fps |
| `spectrumNumCh` / `waveformNumCh` | Audio data requested — `chNone` or `chStereo` |
| `width` / `height` | Window size in pixels |
| `bitsPerPixel` | Color depth — `bppPaletted`, `bpp16`, or `bpp32` |
| `scrollLength` | Max chars of song title to scroll in OSD |

### Beat Detection (`analyze()`)

The beat detection algorithm (ported from MilkDrop / Vis_MegaSDK v1.03) lives in `BEAT.C/H`. It analyses FFT output into three bands (bass, mids, treble) and detects beats by comparing short-term vs long-term averages. Processes all audio channels (mono, stereo, 5.1, 7.1) independently.

### Module Callbacks

Each module can optionally define:
- `initModule` — called after Allegro init (e.g., load bitmaps)
- `quitModule` — called during cleanup (e.g., free resources)
- `inputHandler` — receives WM_CHAR keys (e.g., W/T toggles for Boom Box)
- `readConfig` / `writeConfig` — module-specific INI settings

## Code Conventions

- **Language:** C (C89 with GNU extensions where needed)
- **Brace style:** K&R — opening brace on same line
- **Indentation:** 4 spaces
- **Naming:** camelCase
- **Comments:** avoid unless explaining non-obvious design decisions

## Adding a New Visualization Module

1. Create `vis_myvis.c` with an `int visMyvisRender(struct winampVisModule*)` function
2. Add a `__attribute__((constructor)) static void registerMyvis()` function at the end that creates a `VisModuleDef` and calls `registerModule(&moduleDef)`
3. Add the compilation step in `BUILD.BAT`
4. Module is automatically registered at DLL load time — no other files need changes

## Debugging

- Set `DEBUG=1` environment variable before running `BUILD.BAT` for debug symbols
- Plugin configuration is persisted to `alvis.ini` in the Winamp directory
- On-screen display (OSD) can be toggled with Y key

## Compatibility Notes

- **Target:** Winamp 5.x on Windows
- **Allegro:** Dynamic by default (`-lalleg`, requires `alleg42.dll`); use `set STATIC=1` for static link
- **Build:** MinGW 32-bit only (Allegro 4 Windows port)
- **Channels:** Processes up to 8 channels for beat detection (stereo, 5.1, 7.1); displays L/R
