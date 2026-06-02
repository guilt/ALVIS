# ALVIS — Allegro Visualization Plugin

by **Karthik Kumar Viswanathan** (@guilt) & **Nikhil Karnad** (@scribbleink)

## Contents
- [Introduction](#introduction)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Build Dependencies](#build-dependencies)
- [Building from Source](#building-from-source)
- [License](#license)

---

## Introduction

ALVIS is a Winamp 5 visualization plugin built with the Allegro graphics library. It provides five
distinct visualization modes that react to music playback in real time.

---

## Features

| # | Mode | Description |
|---|------|-------------|
| 1 | **WaveForm Analyser** | Oscilloscope display for left/right channels |
| 2 | **Spectrum Analyser** | Spectral frequency graph |
| 3 | **VU Analyser** | Audio level meter with bars and lines |
| 4 | **The Grind** | Beat-driven animated ellipses |
| 5 | **Boom Box - Explosion** | Radial speaker explosion with waveform or spectrum |

### Controls (in the visualization window)

| Key | Function |
|-----|----------|
| Y | Toggle On-Screen Display (OSD) |
| T | Toggle Boom Box background |
| W | Switch Boom Box mode (waveform / spectrum) |

---

## Installation

Run the NSIS installer (`alvis.exe` from `DIST\`) or manually copy to Winamp's `Plugins\` directory:

- `vis_alvis.dll` — the visualization plugin
- `alleg42.dll` — Allegro runtime library (required)
- `boom.bmp` — Boom Box background image
- `alvis.htm` — help page

Select ALVIS from Winamp's Visualization menu (Ctrl+P → Plug-ins → Visualization).

---

## Usage

Once selected in Winamp, ALVIS presents five visualization modes accessible via Winamp's visualization cycling (default hotkey: K). Each mode runs in its own window with OSD support showing the current song title, position, and length.

---

## Build Dependencies

- **MinGW/GCC** — 32-bit compiler
- **Allegro 4.x** — graphics library
- **Winamp SDK** — Vis_MiniSDK v1.01 / wa502_sdk (included: `vis.h`, `wa_ipc.h`)

---

## Building from Source

```
BUILD.BAT              (dynamic, default)
set STATIC=1 && BUILD.BAT   (static link)
```

Output goes to `DIST\`. See [DEVELOP.MD](DEVELOP.MD) for details.

---

## License

This project is licensed under the MIT License — see [LICENSE.MD](LICENSE.MD).
