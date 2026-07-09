# Changelog

All notable changes to the Doodle Engine will be documented in this file.

The project adheres to [Semantic Versioning](https://semver.org/).

---

## [1.2.0] - 2026-07-07

### Added
- **Dynamic Memory Arena (`arena.h`)**: Replaced standard static allocations with a linked-list region-based memory arena. Supports dynamic multi-region growth, alignment validation, in-place reallocations (`ArenaRealloc`), snapshot markers (`ArenaGetMarker`), rewinding (`ArenaRewind`), trimming (`ArenaTrim`), and dynamic array growth macros.
- **Stereo & LFO Synthesizer (`daudio.h/.c`)**:
  - Support for stereo audio output with constant power panning.
  - Frequency pitch slides for continuous pitch sweeps.
  - Low-Frequency Oscillators (LFO) for frequency modulation (vibrato) and amplitude modulation (tremolo).
  - Low-pass audio filtering with configurable cutoff frequencies.
  - Thread-safe mutex locking for synth voice manipulation.
- **Trigonometry Lookup Table (LUT) (`fast_math.h`)**: Pre-computed 360-degree fast sine/cosine implementation to replace computationally expensive standard library trig calls.
- **Automatic Shader Hot-Reloading**: Watcher function `CheckShaderUpdates` inside `cache.c` automatically re-compiles and reloads custom GLSL fragment shaders when they are modified on disk.

### Changed
- **UINode Memory Footprint**: Redesigned `UINode` and `StyleProps` string buffers to point directly to pooled `const char*` fields allocated in the centralized `dom_arena`. This reduced the size of the node struct by **90%** (from 1.4 KB down to ~220 bytes).
- **DOM Arena Size**: Reduced size limits of the layout tree `dom_arena` from **2.0 MB to 512 KB** to save overhead.
- **Dynamic Asset Caching**: Updated cache storage (textures, fonts, sounds, shaders, music) from static arrays to dynamic hash tables using the FNV-1a hashing algorithm.
- **Particle Pool size**: Reduced `MAX_PARTICLES` pool limit from **2,048 to 512** for optimized memory allocation.
- **Flexbox Sort Optimization**: Replaced bubble sort with standard library `qsort` for sorting specificity in the CSS parser.
- **CLI Packager Tool**: Ported `cli.py` to `argparse` with improved option support (`--name` / `-n`, `--console` flags) and dynamic search globbing to find compiled Python libraries (`_doodle*.pyd` / `_doodle*.so`), preventing PyInstaller packaging failures.
- **Compilation Tooling**: Updated `setup.py` with GCC compiler sections flags (`-ffunction-sections`, `-fdata-sections`, `-Wl,--gc-sections`) to strip dead code, reducing executable sizes.

### Fixed
- **macOS Compilation**: Included `<stddef.h>` in `mparser.h` to define `size_t`, resolving compiler errors on macOS/Clang.
- **FBO Camera Rendering Bypass**: Optimized camera view rendering. If camera mode is active but no custom post-processing shader is specified in the CSS, the engine bypasses Framebuffer Object (FBO) creation and blits directly to the window backbuffer, saving GPU overhead.
- Corrected empty string pointer assignments for initialized layout nodes, avoiding null pointer dereferences.

---

## [1.1.0] - 2026-06-30

### Added
- **2D Camera Support**: Added `<view camera="true">` containers and `doodle.setCamera` API to move, zoom, and rotate the camera.
- **Particle System**: Added particle emitter pools with `doodle.spawnParticles` API.
- **Camera Screen Shake**: Added `doodle.shakeCamera` function for screen shake effects.
- **Animation (Tweens)**: Integrated `doodle.animate` API supporting linear, quad_in, quad_out easing curves.
- **Batch Update System**: Added `doodle.batchProcess` to combine multiple updates and collision queries in a single Python→C transition.
- **Developer Console**: Added runtime interactive console toggled with the `~` key.

### Changed
- Refactored C-Python API bindings to support both `camelCase` and `snake_case` twins.
- Exposed full type stubs (`__init__.pyi`) for autocomplete in IDEs.

---

## [1.0.0] - 2026-06-20

### Added
- Initial release of the Doodle UI & 2D Game Engine.
- Custom single-pass HTML template and CSS parsers.
- Simplified Flexbox layout solver (px, %, grow, fit, margins, paddings).
- GPU-accelerated drawing backend using Raylib (views, text, circles, lines, images, custom shaders).
- Primitive collision engine (Rect-Rect, Circle-Circle, Circle-Rect).
- Polyphonic procedural audio synthesizer and sound playback.
