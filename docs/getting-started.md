# Getting Started with Doodle Engine

This guide walks you through installing Doodle, building your first app, and understanding the core workflow.

---

## Installation

### Prerequisites

| Requirement | Details |
|-------------|---------|
| **Python** | 3.11 or newer |
| **C Compiler** | MSYS2/MinGW-w64 on Windows, GCC on Linux |
| **Raylib** | Included in `third_party/` (no separate install needed) |

### Step 1: Clone & Setup

```bash
git clone https://github.com/user/Doodle.git
cd Doodle
python -m venv .venv
```

### Step 2: Compile the C Extension

```bash
# Windows (PowerShell)
.venv\Scripts\python.exe setup.py build_ext --inplace

# Linux
.venv/bin/python setup.py build_ext --inplace
```

This compiles all C source files in `src/` and produces `doodle/_doodle.pyd` (Windows) or `doodle/_doodle.so` (Linux).

### Step 3: Verify

```bash
# Windows
.venv\Scripts\python.exe -c "import doodle; print('Doodle loaded!')"

# Linux
.venv/bin/python -c "import doodle; print('Doodle loaded!')"
```

---

## Your First Doodle App

Every Doodle app consists of three files:

```text
my-app/
├── layout.html    # UI structure (what exists)
├── styles.css     # Visual styling (how it looks)
└── main.py        # Logic & behavior (what it does)
```

### Step 1: Create `layout.html`

```html
<view id="app">
    <text id="greeting">Hello, Doodle!</text>
    <text id="counter">Clicks: {{ count }}</text>
    <button id="click-btn" onclick="on_click">Click Me</button>
</view>
```

### Step 2: Create `styles.css`

```css
#app {
    flex-direction: column;
    width: 100%;
    height: 100%;
    background-color: #1a1a2e;
    justify-content: center;
    align-items: center;
}

#greeting {
    font-size: 36;
    color: #e94560;
    margin-bottom: 20;
}

#counter {
    font-size: 24;
    color: #00ffcc;
    margin-bottom: 30;
}

#click-btn {
    width: 200px;
    height: 50px;
    background-color: #e94560;
    color: #ffffff;
    font-size: 20;
    border-radius: 8;
    text-align: center;
}

#click-btn:hover {
    background-color: #ff6b81;
}
```

### Step 3: Create `main.py`

```python
import doodle

state = {"count": 0}

def on_click():
    state["count"] += 1
    doodle.playSynth(440.0 + state["count"] * 20, 0.08, doodle.WAVE_TRIANGLE)
    doodle.spawnParticles(400, 300, 15, "#e94560", 3.0, 0.5)

doodle.setEventContext({"on_click": on_click})

doodle.run(
    layout="layout.html",
    style="styles.css",
    width=800,
    height=600,
    title="My First Doodle App",
    state=state
)
```

### Step 4: Run It

```bash
.venv\Scripts\python.exe main.py
```

You should see a centered UI with a greeting, a live click counter, and a button that plays a synthesized tone and spawns particles on each click.

---

## Core Concepts

### The Render Loop

Doodle runs a 60 FPS game loop. Each frame:

1. **Hot-reload check** — If `layout.html` or `styles.css` changed on disk, the DOM is rebuilt automatically
2. **Hover state update** — CSS `:hover` styles are applied/removed based on mouse position
3. **Tick callback** — Your registered Python function runs (game logic, input handling, animations)
4. **Layout recompute** — If any positions/sizes changed, the flexbox solver re-runs
5. **Draw** — The entire DOM tree is rendered to the GPU

### Reactive Templates

Text nodes containing `{{ variable }}` are automatically updated each frame from the `state` dictionary:

```html
<text id="score">SCORE: {{ score }}</text>
```

```python
state = {"score": 0}
state["score"] += 100  # Text updates automatically next frame
doodle.run(..., state=state)
```

### Event System

Doodle supports two styles of event handling:

**Programmatic (in Python):**
```python
doodle.addEventListener("btn", "click", lambda: print("Clicked!"))
doodle.addEventListener("btn", "hover", lambda: print("Hovered!"))
```

### Hot Reloading

While your app is running, edit `layout.html` or `styles.css` and save. Doodle detects the file change and instantly rebuilds the DOM, preserving node positions set by Python. This makes UI iteration extremely fast.

---

## Next Steps

- **[API Reference](api-reference.md)** — Complete function and class documentation
- **[Layout & Styling Guide](layout-and-styling.md)** — All HTML tags and CSS properties
- **[Architecture Overview](architecture.md)** — How the C engine works internally
- **[Cheatsheet](cheatsheet.md)** — Quick lookup card
