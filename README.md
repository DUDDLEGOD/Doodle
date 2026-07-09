# Doodle Engine

### Hardware-Accelerated Hybrid C-Python UI & 2D Game Engine

<p align="center">
  <strong>Build games, creative tools, and interactive apps using HTML-like layouts, CSS styling, and Python scripting — all rendered natively on the GPU.</strong>
</p>

---

Doodle is a high-performance, lightweight engine that combines a **native C rendering core (powered by Raylib)** with a **highly reactive Python scripting layer**. Instead of embedding a browser, Doodle parses HTML-like templates and CSS stylesheets in a single pass, mapping them directly to GPU-accelerated graphics, procedural audio, and real-time collision systems.

## ✨ Key Features

| Category | Features |
|----------|----------|
| **Rendering** | GPU-accelerated 2D drawing, rounded rectangles, circles, lines, images, custom GLSL shaders, particle system |
| **Layout** | Flexbox engine (row/column, justify-content, align-items), absolute positioning, percentage/pixel/grow/fit sizing |
| **Styling** | Full CSS property system, hover states, dynamic runtime style changes, custom fonts, border-radius, opacity |
| **Input** | Keyboard polling (down/pressed), mouse position/buttons/wheel/cursor, node click & hover detection |
| **Audio** | Polyphonic procedural synthesizer (5 waveforms, ADSR envelopes, stereo panning, pitch slide, vibrato, tremolo, low-pass filter), cached sound/music files |
| **Collision** | Rect-Rect, Circle-Circle, Circle-Rect detection, class-group collision queries, batch collision processing |
| **Animation** | Tween engine with easing curves (linear, quad_in, quad_out) |
| **Scripting** | Full Python OOP node wrappers, property accessors (`node.x += 10`), reactive `{{ template }}` data binding |
| **Dev Tools** | Built-in FPS/draw-call/RAM profiler, runtime Python console (`~` key), hot-reload for layout, styles, and shaders |
| **Memory & Perf**| Linked-list memory arenas, O(1) hash cache lookup, Trigonometry LUT, FBO render bypass, 90% reduced node layout structs |
| **DX** | camelCase + snake_case dual API, full `.pyi` type stubs for IDE autocomplete, descriptive error messages with layout line numbers |

---

## 🚀 Getting Started

### Requirements
- **OS**: Windows (x64) or Linux
- **Python**: 3.11+
- **C Toolchain**: MSYS2/MinGW-w64 (Windows) or GCC (Linux)

### Quick Setup

```bash
# Clone and enter the repository
git clone https://github.com/user/Doodle.git
cd Doodle

# Create virtual environment
python -m venv .venv

# Compile the native C extension
# Windows (PowerShell):
.venv\Scripts\python.exe setup.py build_ext --inplace

# Linux:
.venv/bin/python setup.py build_ext --inplace
```

### Run the Breakout Demo

```bash
# Windows:
.venv\Scripts\python.exe examples\breakout\main.py

# Linux:
.venv/bin/python examples/breakout/main.py
```

---

## 📖 How It Works

Doodle apps are built with three files:

### 1. `layout.html` — Define your UI structure
```html
<view id="app">
    <view id="hud">
        <text id="score">SCORE: {{ score }}</text>
        <text id="lives">LIVES: {{ lives }}</text>
    </view>
    <view id="game-area" camera="true">
        <view id="player"></view>
        <view id="ball"></view>
        <circle id="coin" radius="12" color="#facc15"></circle>
    </view>
    <view id="game-over" style="display: none;">
        <text id="msg">GAME OVER</text>
        <button id="restart-btn" onclick="restart">RESTART</button>
    </view>
</view>
```

### 2. `styles.css` — Style everything with familiar CSS
```css
#app {
    flex-direction: column;
    width: 100%;
    height: 100%;
    background-color: #0a0a0a;
}

#hud {
    flex-direction: row;
    justify-content: space-between;
    padding: 12 24;
    background-color: #111;
    font-size: 18;
    color: #00ffcc;
}

#player {
    width: 80px;
    height: 16px;
    background-color: #00ffcc;
    border-radius: 4;
}

#ball {
    width: 16px;
    height: 16px;
    background-color: #ffffff;
    border-radius: 8;
}
```

### 3. `main.py` — Script your logic in Python
```python
import doodle

state = {"score": 0, "lives": 3}

player = doodle.getNode("player")
ball = doodle.getNode("ball")

ball_dx, ball_dy = 5.0, -5.0

def tick():
    global ball_dx, ball_dy

    # Move player with keyboard
    if doodle.isKeyDown(263):  # Left arrow
        player.x -= 8
    if doodle.isKeyDown(262):  # Right arrow
        player.x += 8

    # Move ball
    ball.x += ball_dx
    ball.y += ball_dy

    # Bounce off paddle
    if doodle.checkCollision("ball", "player"):
        ball_dy = -abs(ball_dy)
        doodle.playSynth(440.0, 0.1, doodle.WAVE_SQUARE)
        doodle.shakeCamera(4.0, 0.15)
        doodle.spawnParticles(ball.x, ball.y, 15, "#00ffcc", 3.0, 0.4)

doodle.registerTickCallback(tick)
doodle.run(layout="layout.html", style="styles.css", width=800, height=600, state=state)
```

---

## 🎮 Python API Overview

### Engine Lifecycle
```python
doodle.run(layout, style, width, height, title, state)  # Start the engine
doodle.registerTickCallback(fn)                          # Per-frame update function
```

### Node Manipulation (OOP)
```python
node = doodle.getNode("player")    # Get a node wrapper

node.x += 10                       # Direct position access
node.y = 300
node.position = (100, 200)         # Tuple assignment
node.width = 80                    # Resize via CSS
node.text = "Hello!"               # Update inner text
node.visible = False               # Hide from rendering
node.show() / node.hide()          # Visibility methods
node.style.background_color = "red"  # Dynamic CSS properties
```

### Input
```python
doodle.isKeyDown(key_code)          # Held key check
doodle.isKeyPressed(key_code)       # Single-frame press
doodle.getMousePosition()           # (x, y) tuple
doodle.isMouseButtonPressed(0)      # Left click
doodle.isNodeClicked("btn")         # Click detection on DOM node
doodle.isNodeHovered("btn")         # Hover detection on DOM node
```

### Collision Detection
```python
doodle.checkCollision("a", "b")              # Rect/Circle aware
doodle.getFirstCollision("ball", "bricks")   # Class group query
doodle.batchProcess(positions={...}, collisions=[...])  # Batched C call
```

### Audio
```python
doodle.playSound("sfx/hit.wav")                              # Cached WAV/OGG playback
doodle.playSynth(frequency, duration, wave_type=1, attack=0.01, decay=0.05, sustain=0.5, release=0.05, frequency_slide=0.0, vibrato_speed=0.0, vibrato_depth=0.0, tremolo_speed=0.0, tremolo_depth=0.0, filter_cutoff=0.0, pan=0.0) # Synthesizer
# Waveforms: WAVE_SINE=0, WAVE_SQUARE=1, WAVE_TRIANGLE=2, WAVE_SAWTOOTH=3, WAVE_NOISE=4
```

### Effects
```python
doodle.spawnParticles(x, y, count, color_hex, speed, lifetime)  # Particle burst
doodle.shakeCamera(intensity, duration)                         # Screen shake
doodle.animate("node", target_x=100, duration=0.5, ease="quad_out")  # Tweens
```

### Camera
```python
doodle.setCamera(target_x, target_y, offset_x, offset_y, zoom, rotation)
```

---

## 🔧 Developer Tools

Press **`~`** (tilde) at runtime to open the built-in developer console. Execute Python commands live:

```
>>> getNode("ball").x += 100
>>> spawnParticles(400, 300, 50, "#ff0", 5.0, 1.0)
```

The profiler overlay shows FPS, draw calls, particle count, and CPU time per frame.

---

## 📁 Repository Structure

```text
Doodle/
├── doodle/                    # Python package
│   ├── __init__.py            # OOP wrappers, tweens, events, templates
│   ├── __init__.pyi           # Full type stubs for IDE autocomplete
│   └── cli.py                 # PyInstaller packaging CLI
├── src/                       # C extension core (compiles to _doodle.pyd)
│   ├── expose_raylib.c        # CPython bindings, main loop, particles
│   ├── mparser.h / .c         # DOM tree, node pool, hash-table lookup
│   ├── html_parser.h / .c     # Single-pass HTML parser with line tracking
│   ├── css_parser.h / .c      # CSS parser with functional registry pattern
│   ├── layout.h / .c          # Flexbox layout solver
│   ├── renderer.h / .c        # GPU draw traversal, shaders, z-sorting
│   ├── daudio.h / .c          # Polyphonic synth, ADSR envelope generator
│   ├── particles.h / .c       # Object-pooled particle system
│   ├── profiler.h / .c        # FPS counter, draw call tracker, dev console
│   ├── cache.h / .c           # Texture, sound, shader, font caching
│   ├── color.h / .c           # Hex/named color parser
│   ├── unit.h / .c            # CSS unit parser (px, %, grow, fit)
│   ├── string_utils.h / .c    # String trimming and helpers
│   └── dom_utils.h / .c       # Tree traversal utilities
├── examples/                  # Demo applications
│   └── breakout/              # Breakout game with CRT shader
├── docs/                      # Documentation
│   ├── getting-started.md     # Installation & first app tutorial
│   ├── api-reference.md       # Complete API reference
│   ├── layout-and-styling.md  # HTML tags & CSS properties guide
│   ├── architecture.md        # Engine internals & C architecture
│   └── cheatsheet.md          # Quick reference card
├── third_party/               # Static Raylib binaries
├── setup.py                   # Build script
├── pyproject.toml             # Package metadata
└── README.md                  # You are here!
```

---

## ⚡ Performance

Doodle is engineered for speed with several custom optimizations:

- **Hash-table node lookup** — O(1) average case for `FindNodeById` instead of tree traversal
- **Object-pooled particles** — Pre-allocated particle array, no runtime allocation
- **Batched C calls** — `batchProcess()` combines position/text/visibility/collision updates into one Python→C roundtrip
- **Fast math** — SSE3 intrinsics, cached inverse values for division elimination, direct phase-boundary square waves
- **Precompiled templates** — `{{ score }}` compiles once to Python `{score}` format strings
- **Lazy layout** — Layout recomputation only when `layout_dirty` flag is set

---

## 📄 License

MIT License. See [LICENSE](LICENSE) for details.
