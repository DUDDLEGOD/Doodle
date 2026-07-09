# Doodle Engine — Architecture Overview

This document describes the internal architecture of the Doodle Engine's C core, how it compiles, and how data flows from HTML/CSS files through the rendering pipeline to the screen.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Python Layer (doodle/)                   │
│  __init__.py: OOP wrappers, tweens, events, templates       │
│  __init__.pyi: Type stubs for IDE autocomplete              │
└──────────────────────┬──────────────────────────────────────┘
                       │ CPython C API
┌──────────────────────▼──────────────────────────────────────┐
│                  C Extension (_doodle.pyd)                   │
│                                                              │
│  ┌──────────┐  ┌───────────┐  ┌──────────┐  ┌───────────┐  │
│  │  HTML    │  │   CSS     │  │  Layout  │  │ Renderer  │  │
│  │  Parser  │→ │  Parser   │→ │  Solver  │→ │  (GPU)    │  │
│  └──────────┘  └───────────┘  └──────────┘  └───────────┘  │
│                                                              │
│  ┌──────────┐  ┌───────────┐  ┌──────────┐  ┌───────────┐  │
│  │  Node    │  │  Cache    │  │ Particle │  │  Synth    │  │
│  │  Pool    │  │  Manager  │  │  System  │  │  Engine   │  │
│  └──────────┘  └───────────┘  └──────────┘  └───────────┘  │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │         expose_raylib.c (Main Loop & Bindings)       │   │
│  └──────────────────────────────────────────────────────┘   │
└──────────────────────┬──────────────────────────────────────┘
                       │ Raylib API
┌──────────────────────▼──────────────────────────────────────┐
│              Raylib (OpenGL / GPU / Audio)                    │
└──────────────────────────────────────────────────────────────┘
```

---

## C Source Modules

### `expose_raylib.c` — Main Entry Point

The central module that:
- Defines all CPython method bindings (`PyMethodDef DoodleMethods[]`)
- Contains the `doodle_run()` function with the main 60 FPS render loop
- Manages the hot-reload file watcher
- Hosts the `GetNodeOrRaise()` helper for safe node lookups
- Contains the `doodle_batch_process()` function for batched updates

**Main loop flow per frame:**
1. Check file modification times for hot-reload
2. Update hover states on all nodes
3. Call Python tick callback
4. Recompute layout if dirty
5. Update music streams
6. `BeginDrawing()` → Draw DOM tree → Draw particles → Draw dev tools → `EndDrawing()`

### `mparser.h / .c` — DOM Tree & Node Management

Defines the core data structures and memory layout:
- **`UINode` struct** — Optimized to use pooled string pointers (`const char*`) instead of static buffers for `id`, `class_name`, `text_content`, `asset_path`, `font_path`, `shader_path`, etc. This reduces each node's memory footprint by 90% (from 1.4 KB to ~220 bytes).
- **`StyleProps` struct** — Holds CSS style values (colors, sizing, padding, margin, flex, fonts, shaders, transforms).
- **`LayoutBox` struct** — Stores computed absolute rendering bounds (`x`, `y`, `width`, `height`).
- **Memory Optimization** — Allocates nodes and dynamic strings (`DOMStrDup`) inside a centralized `dom_arena` (sized at 512 KB to minimize RAM overhead).
- **Lookup Registry** — Hash-table based `FindNodeById` for O(1) lookups instead of slow recursive tree searches.
- **Tree Operations** — Helper routines like `CreateNode`, `AddChild`, `RemoveNode`, and `FreeNode`.

### `html_parser.h / .c` — HTML Parser

Single-pass scanner that:
- Tokenizes HTML tags with attributes
- Builds the `UINode` tree hierarchy using a parent stack
- Parses inline `style="..."` attributes through the CSS registry
- Tracks line numbers for error reporting
- Handles self-closing tags, class names, event attributes

### `css_parser.h / .c` — CSS Parser & Registry

**Functional Registry Pattern:** Instead of a giant switch statement, CSS properties are mapped to handler functions via a static lookup table:

```c
typedef void (*CSSPropertyHandler)(UINode* node, const char* value);

typedef struct {
    const char* property_name;
    CSSPropertyHandler handler;
    int affects_layout;  // If true, triggers layout recomputation
} CSSMap;

CSSMap css_registry[] = {
    {"background-color", handle_bg_color, 0},
    {"width",            handle_width,    1},
    {"flex-direction",   handle_flex_direction, 1},
    // ... 30+ properties
};
```

The parser supports:
- Tag selectors (`text { ... }`)
- Class selectors (`.brick { ... }`)
- ID selectors (`#player { ... }`)
- Hover pseudo-class (`#btn:hover { ... }`)
- Comment skipping (`/* ... */`)

### `layout.h / .c` — Flexbox Layout Solver

Implements a simplified flexbox algorithm:
1. **Sizing pass** — Resolve `width`/`height` from fixed, percentage, grow, or fit values
2. **Content sizing** — Measure text content to determine `fit` dimensions
3. **Flex distribution** — Distribute remaining space among `grow` children proportionally
4. **Alignment** — Apply `justify-content` and `align-items` along main/cross axes
5. **Absolute positioning** — Position nodes with `position: absolute` or Python-set coordinates

Layout only recomputes when `layout_dirty` flag is set (by position changes, text updates, visibility toggles, or style modifications).

### `renderer.h / .c` — GPU Draw Traversal

Walks the DOM tree and draws each node:
- **Views**: `DrawRectangleRounded` with background color, border, and border-radius
- **Text**: `DrawTextEx` with custom fonts, size, and alignment
- **Images**: `DrawTexturePro` with cached textures and tint
- **Circles**: `DrawCircle` with radius and color
- **Lines**: `DrawLineEx` with thickness and color
- **Shaders**: Render-to-texture with custom fragment shader post-processing

Supports:
- Z-index sorting (children sorted by `z-index` before drawing)
- Opacity via alpha blending
- Rotation transforms
- Camera-aware rendering (`BeginMode2D` / `EndMode2D`)
- Hover state style swapping
- **FBO Camera Bypass** — If a container uses a camera but has no custom post-processing shader, the renderer bypasses Framebuffer Object (FBO) creation and texture blitting, rendering directly to the backbuffer to save GPU cycles.

### `daudio.h / .c` — Polyphonic Synthesizer

Multi-voice audio engine featuring:
- **8 simultaneous voices** with independent phase accumulators and thread-safe mutex operations.
- **5 waveforms**: Sine, Square, Triangle, Sawtooth, White Noise.
- **ADSR envelope generator** per voice (attack, decay, sustain, release).
- **Advanced parameters** per voice: Stereo panning (constant power panning), pitch slides, vibrato (pitch LFO), tremolo (amplitude LFO), and low-pass filtering.
- **Fast-path optimizations**: Fast phase-boundary comparison for square waves, pre-calculated 360-degree trigonometry lookup table (`fast_math.h`) for sine waves, and cached inverse lifetimes to eliminate division in the audio callback.
- **Audio callback** fills Raylib's audio stream buffer dynamically each frame.

### `particles.h / .c` — Object-Pooled Particle System

- Pre-allocated pool of **512 particles** (zero runtime allocation footprint).
- Each particle tracks: position, velocity, color, lifetime, max_lifetime.
- Velocity angles use `FastRandFloat` (via xorshift128 PRNG) and precalculated fast trigonometry.
- Opacity fades out linearly based on remaining lifetime.
- Rendered on the GPU as 3×3 pixel rectangles.

### `cache.h / .c` — Asset Caching

Caches loaded assets to prevent redundant disk reads:
- Uses **dynamic hash tables** backed by a shared memory arena instead of fixed-size arrays.
- Caches:
  - **Textures**: Keyed by file path, loaded via `LoadTexture`.
  - **Sounds**: Keyed by file path, loaded via `LoadSound`.
  - **Shaders**: Keyed by file path, compiled via `LoadShader`. Includes **automatic runtime shader reloading** (`CheckShaderUpdates`) when files change.
  - **Fonts**: Keyed by path + size, loaded via `LoadFontEx`.
- All cached resources are cleaned up on window close.

### `profiler.h / .c` — Developer Tools

Built-in overlay showing:
- FPS counter
- Draw call counter
- Active particle count
- CPU time per tick (milliseconds)
- Memory usage estimate

Also implements the runtime Python console:
- Toggle with `~` key
- Text input with cursor and backspace handling
- Scrollable output history
- Calls registered Python callback for command evaluation

### Utility Modules

- **`color.h / .c`** — Parses hex (`#ff5733`), named (`red`), and `rgba()` color strings to Raylib `Color` structures.
- **`unit.h / .c`** — Parses CSS units (`100px`, `50%`, `grow`, `fit`) to numeric values.
- **`arena.h`** — Generic growable Memory Arena allocator supporting alignment validation, dynamic region list, snapshots, rewinds, and dynamic array macros.
- **`fast_math.h`** — Precalculated 360-degree trigonometry lookup table for `fast_sin` and `fast_cos`.
- **`string_utils.h / .c`** — String trimming and FNV-1a hashing.
- **`dom_utils.h / .c`** — Tree traversal helpers.

---

## Compilation

The `setup.py` script compiles all `.c` files in `src/` into a single shared library:

```
src/*.c  →  gcc  →  _doodle.cp311-win_amd64.pyd  →  doodle/_doodle.pyd
```

Linked against:
- `raylib` (static library in `third_party/`)
- `gdi32`, `winmm`, `user32`, `shell32`, `psapi` (Windows system libraries)
- `python311` (CPython interpreter)

Compiler flags: `-std=c99 -O3 -ffast-math -msse3`

---

## Data Flow

```
layout.html  ──→  ParseHTML()  ──→  UINode tree
                                        │
styles.css   ──→  LoadAndApplyCSS()  ───┘
                                        │
                                        ▼
                               ComputeLayout()
                                        │
                                        ▼
                                DrawUINode()  ──→  GPU (OpenGL)
                                        │
Python tick() ──→ setPosition() ────→ layout_dirty = 1
                  updateText()  ────→ layout_dirty = 1
                  setStyle()    ────→ layout_dirty = 1
```

The `layout_dirty` flag ensures the expensive `ComputeLayout()` only runs when something actually changed.

---

## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Node lookup by ID | O(1) average | Hash-table with linear probing |
| Layout recompute | O(n) | Only when dirty flag is set |
| Collision check | O(1) per pair | Direct Raylib math functions |
| Group collision | O(n) | Walks tree filtering by class |
| Particle update | O(k) | k = active particles (max 512) |
| Synth voices | O(8) per audio buffer | Fixed 8-voice polyphony |
| CSS application | O(m) per property | m = registry size (~30) |
