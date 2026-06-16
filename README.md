# Doodle Engine 🎨🚀
### Hardware-Accelerated Hybrid C-Python UI & 2D Game Engine

Doodle is a high-performance, lightweight UI and 2D game engine combining a **native C rendering core (powered by Raylib)** with a **highly reactive Python scripting layer**.

By bypassing heavy browser runtimes (like Chromium/Electron), Doodle parses dynamic HTML-like templates and CSS stylesheets in a single pass directly to GPU-accelerated graphics, procedural audio synths, and collision meshes.

---

## 📂 Repository Organization

```text
Doodle/
├── docs/                   # Full reference specifications & requirements
│   ├── cheatsheet.md       # API & layout syntax quick reference
│   ├── requirements.md     # Engine specifications and architecture blueprint
│   └── unused_reference.md  # Unused HTML/CSS/C engine features reference
├── doodle/                 # Python package wrapper
│   ├── __init__.py         # Tween animations, reactive state binding, event emitter
│   └── cli.py              # PyInstaller packager cli
├── examples/               # Game demonstrations
│   └── breakout/           # Breakout game demo
│       ├── layout.html     # Game view DOM nodes & inline handlers
│       ├── styles.css      # Retro arcade UI styles, flexbox, and crt shaders
│       ├── shaders/        # Custom GLSL Fragment shaders
│       └── main.py         # Collision, input, and game state script
├── src/                    # C-Extension Core (complies to _doodle.pyd)
│   ├── setup.py            # Setuptools compilation script
│   ├── expose_raylib.c     # Python-to-C bindings, Raylib window, and particle pool
│   ├── mparser.h / .c      # n-ary DOM parser, CSS registry handler, and layout box solver
│   ├── dutils.h / .c       # Custom color parser, fast unit converter, and math helpers
│   └── daudio.h / .c       # Polyphonic synth, multi-voice ADSR envelope generator
├── third_party/            # Static Raylib binaries & dependencies
└── README.md               # You are here!
```

---

## 🛠️ Getting Started & Compilation

### 1. Requirements
* **OS**: Windows (x64) / linux
* **Python**: Python 3.11+ (with virtual environment capability)
* **C Toolchain**: MSYS2/MinGW-w64 (`C:\msys64\ucrt64\bin` for `gcc` and compilation libraries) / linux cc compiler

### 2. Quick Setup & Build
From the repository root directory, run the setuptools compilation with your virtual environment's python. Specify the compiler:

```bash
# Add GCC to PATH (if not global)
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH

# Compile the native C extension
cd src
..\.venv\Scripts\python.exe setup.py build_ext --inplace -c mingw32

# Copy the compiled pyd module to the doodle library
cd ..
Copy-Item -Path "src\_doodle.cp311-win_amd64.pyd" -Destination "doodle\_doodle.pyd" -Force
```

### 3. Run the Breakout Demo
Launch the breakout game:
```bash
cd examples/breakout
..\..\.venv\Scripts\python.exe main.py
```

---

## 🏗️ XML Markup (`layout.html`) & CSS (`styles.css`)

Doodle supports layout-driven layouts with traditional tag structures and inline event bindings.

### Core Elements
* `<view>`: Structural container. Standard flex container or a 2D camera boundary using `<view camera="true">`.
* `<text>`: Text layout with reactive state-bound variables like `SCORE: {{ score }}`.
* `<image>`: Renders cached bitmaps using `src="..."`.
* `<button>`: Clickable target with mouse state callbacks.
* `<circle>`: Renders shapes using `radius` and `color` parameters.
* `<line>`: Renders vectors using `x2`, `y2`, `thickness`, and `color`.

### Event Hooks
Directly attach Python functions inline:
* `onclick="python_function_name"`
* `onhover="python_function_name"`

### Layout Styles
* **Sizing Rules**: `width` / `height` support pixels (`100px`), percentages (`50%`), growth parameters (`grow`), and content sizing (`fit`).
* **Flexbox Attributes**: `display: flex`, `flex-direction` (`row` | `column`), `justify-content` (`center` | `space-between` | `space-around`), `align-items`.
* **Cosmetics**: `background-color`, `border-radius`, `border-color`, `border-width`, `opacity`, `font-family`, `font-size`.
* **Juice Shaders**: Add custom GLSL fragment shaders directly to views using `shader-path: "shaders/crt.fs"`.

---

## 🐍 Python OOP APIs

### Initialization
```python
import doodle

# Game state passed for template rendering
state = {"score": 0, "lives": 3}

doodle.run(
    layout="layout.html",
    style="styles.css",
    width=800,
    height=600,
    title="Game Window",
    state=state
)
```

### Node Manipulation
```python
# Fetch reference
paddle = doodle.get_node("paddle")

# Read/Write positions
paddle.position = (350, 500)
paddle.x += 10.0

# Easing animations
doodle.animate("paddle", target_x=350, target_y=500, duration=0.4, ease="quad_out")
```

### Input Polling
```python
# Keys
doodle.is_key_down(263)      # Left arrow key code
doodle.is_key_pressed(82)    # R key code

# Mouse
mx = doodle.get_mouse_x()
doodle.set_mouse_cursor(4)   # Changes mouse pointer style
```

---

## 🔊 Polyphonic Procedural Sound Synthesizer

Doodle includes an integrated multi-voice synthesizer utilizing **ADSR Envelopes** to play retro sound waves without lagging the update tick loop.

```python
# Play procedural tone
doodle.play_synth(
    freq=440.0,
    duration=0.15,
    wave_type=doodle.WAVE_TRIANGLE,
    attack=0.01,
    decay=0.05,
    sustain=0.3,
    release=0.05
)
```

**Wave Types**: `WAVE_SINE` (`0`), `WAVE_SQUARE` (`1`), `WAVE_TRIANGLE` (`2`), `WAVE_SAWTOOTH` (`3`), `WAVE_NOISE` (`4`)

---

## ⚡ Performance Optimizations

Doodle is engineered for maximum performance, featuring multiple custom optimizations:
1. **Single-Item DOM Lookup Cache**: Node lookup (`FindNodeById`) caches the pointer of the last requested node. Repetitive state checks (like checking click and hover events in the same frame) resolve in $O(1)$ without scanning the DOM tree.
2. **Precompiled Format Templating**: Template variables `{{ score }}` are compiled once into native Python `{score}` string format structures, converting reactive updates from regex replacements to C-speed string builders.
3. **Loop Division Elimination**: Particle shaders and polyphonic audio generators use cached precalculated inverse values (`inv_max_lifetime` and `phase_increment`) to replace expensive division operations with single-clock multiplication instructions.
4. **Square Wave Optimization**: Square waves skip standard trignometric `sinf` calculations, computing high-low status directly from phase boundaries.
