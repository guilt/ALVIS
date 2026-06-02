# Changelog

## 2026-06-02

- Added LICENSE.MD (MIT License), DEVELOP.MD, CHANGES.MD, .gitignore
- Rewrote README.MD with full project documentation
- Added BUILD.BAT build script (dynamic by default, `set STATIC=1` for static)
- UPX compression of DLL + NSIS installer built automatically to `DIST\`
- NSIS installer: WACUP detection, Add/Remove Programs registration, custom icon
- NSIS installer: Added NSIS Add/Remove Programs registration (uninstall from Settings)
- Removed obsolete WAVISAL.DEV (Dev-C++ project file)
- Renamed `SVIS.C` → `ALVIS.C`, `INS.NSI` → `INSTALL.NSI`
- All source files use 8.3 uppercase filenames for DOS compatibility
- Split code into modules: `COMMON.C` (Winamp IPC), `BEAT.C/H` (beat detection), `VIS_MOD.H` (module system), `VIS_WAVE.C`, `VIS_SPEC.C`, `VIS_VU.C`, `VIS_GRND.C`, `VIS_BOOM.C`
- Each visualization module self-registers via `__attribute__((constructor))` — no central module list
- Each module can opt-in to callbacks: `initModule`, `quitModule`, `inputHandler`, `readConfig`, `writeConfig`
- Boom Box is fully self-contained (bitmap loading, INI config, keyboard input, cleanup) — all data `static` in `VIS_BOOM.C`
- Fixed Grind gray multiplier (was `0.22f` for 8-bit paletted; now full range for 16-bit)
- Fixed `stripDirectory` off-by-one when no backslash in path
- Add a fuzzer

---

## 2004

- Initial release — Winamp 5 visualization plugin with 5 modes
- Based on Vis_MiniSDK v1.01, Vis_MegaSDK v1.03, wa502_sdk, Allegro
- Modes: WaveForm Analyser, Spectrum Analyser, VU Analyser, The Grind, Boom Box - Explosion
