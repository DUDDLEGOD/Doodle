# Doodle Engine — Complete API Reference

This document covers every public function, class, and constant in the `doodle` Python module (v1.1.0).

All functions are available in both **camelCase** (primary) and **snake_case** (backward-compatible alias). Only the camelCase form is documented below; every function has an identical snake_case twin (e.g., `getNode` → `get_node`).

---

## Table of Contents

1. [Engine Lifecycle](#1-engine-lifecycle)
2. [Node Wrappers (OOP)](#2-node-wrappers-oop)
3. [Collision Detection](#3-collision-detection)
4. [Keyboard Input](#4-keyboard-input)
5. [Mouse Input](#5-mouse-input)
6. [Node Interaction](#6-node-interaction)
7. [Style Manipulation](#7-style-manipulation)
8. [Audio & Synthesizer](#8-audio--synthesizer)
9. [Visual Effects](#9-visual-effects)
10. [Camera Control](#10-camera-control)
11. [Window Management](#11-window-management)
12. [Animation (Tweens)](#12-animation-tweens)
13. [Events](#13-events)
14. [Batch Processing](#14-batch-processing)
15. [Debugging & Introspection](#15-debugging--introspection)
16. [Constants](#16-constants)

---

## 1. Engine Lifecycle

### `doodle.run(...)`

Start the engine, open the window, and enter the main render loop.

```python
doodle.run(
    layout: str = "layout.html",
    style: str = "styles.css",
    width: int = 800,
    height: int = 600,
    title: str = "Doodle Engine",
    state: Optional[Dict[str, Any]] = None
) -> None
```

| Parameter | Description |
|-----------|-------------|
| `layout` | Path to the HTML layout file |
| `style` | Path to the CSS stylesheet |
| `width` | Window width in pixels |
| `height` | Window height in pixels |
| `title` | Window title bar text |
| `state` | Dictionary for `{{ template }}` variable binding. Values auto-update in text nodes each frame. |

**Notes:**
- `run()` blocks until the window is closed.
- Automatically changes the working directory to the script's location.
- Enables hot-reloading: editing layout/style files while running triggers an instant rebuild.

---

### `doodle.registerTickCallback(callback)`

Register a function to be called once per frame (at 60 FPS).

```python
def tick():
    # Game logic here
    pass

doodle.registerTickCallback(tick)
```

---

## 2. Node Wrappers (OOP)

### `doodle.getNode(node_id) -> Node`

Returns an OOP wrapper for a DOM node. Can be called before `run()` (lazy initialization).

```python
player = doodle.getNode("player")
```

### `Node` Class Properties

| Property | Type | Read | Write | Description |
|----------|------|------|-------|-------------|
| `node.id` | `str` | ✅ | ❌ | The node's DOM ID |
| `node.x` | `float` | ✅ | ✅ | X position. Supports `node.x += 10` |
| `node.y` | `float` | ✅ | ✅ | Y position |
| `node.position` | `(float, float)` | ✅ | ✅ | `(x, y)` tuple |
| `node.width` | `float` | ✅ | ✅ | Computed width (set updates CSS) |
| `node.height` | `float` | ✅ | ✅ | Computed height |
| `node.w` | `float` | ✅ | ✅ | Alias for `width` |
| `node.h` | `float` | ✅ | ✅ | Alias for `height` |
| `node.text` | `str` | ✅ | ✅ | Inner text content |
| `node.visible` | `bool` | ✅ | ✅ | Visibility state |
| `node.style` | `NodeStyleProxy` | ✅ | ❌ | CSS style accessor |

### `Node` Class Methods

```python
node.show()   # Make visible
node.hide()   # Make invisible
```

### `NodeStyleProxy`

Access and modify CSS properties dynamically. Python `snake_case` names are auto-converted to CSS `dash-names`.

```python
node.style.background_color = "#ff0000"   # Sets background-color
node.style.border_radius = "8"            # Sets border-radius
node.style.font_size = "24"               # Sets font-size

val = node.style.background_color         # Reads the current value
```

### Error Handling

Setting invalid types raises a `TypeError` with the exact layout line number:

```
TypeError: Invalid value for property 'x' on node 'player' (defined at layout.html:L12).
Expected: float or int
Got: 'hello' (type: str)
```

Looking up a non-existent node ID raises `KeyError`:

```
KeyError: "Node with ID 'missing' not found in the DOM tree."
```

---

## 3. Collision Detection

### `doodle.checkCollision(id_a, id_b) -> bool`

Check if two nodes' bounding shapes are overlapping. Automatically handles:
- Rect-Rect collisions
- Circle-Rect collisions
- Circle-Circle collisions

```python
if doodle.checkCollision("ball", "paddle"):
    ball_dy *= -1
```

### `doodle.getFirstCollision(id, group) -> Optional[str]`

Find the first visible node in a CSS class group that collides with the target node.

```python
hit = doodle.getFirstCollision("ball", "bricks")
if hit:
    doodle.removeNode(hit)
```

Returns the colliding node's ID string, or `None`.

---

## 4. Keyboard Input

### `doodle.isKeyDown(key_code) -> bool`

Returns `True` while the key is held down.

### `doodle.isKeyPressed(key_code) -> bool`

Returns `True` only on the frame the key was first pressed.

### Common Key Codes

| Key | Code | Key | Code |
|-----|------|-----|------|
| Left Arrow | `263` | A | `65` |
| Right Arrow | `262` | D | `68` |
| Up Arrow | `265` | W | `87` |
| Down Arrow | `264` | S | `83` |
| Space | `32` | R | `82` |
| Enter | `257` | Escape | `256` |

---

## 5. Mouse Input

| Function | Returns | Description |
|----------|---------|-------------|
| `getMouseX()` | `int` | Mouse X position |
| `getMouseY()` | `int` | Mouse Y position |
| `getMousePosition()` | `(float, float)` | Mouse (x, y) tuple |
| `isMouseButtonDown(btn)` | `bool` | Button held (0=left, 1=right, 2=middle) |
| `isMouseButtonPressed(btn)` | `bool` | Button clicked this frame |
| `getMouseWheelMove()` | `float` | Scroll wheel delta |
| `setMouseCursor(type)` | `None` | Change cursor style (0=default, 4=pointing) |

---

## 6. Node Interaction

### `doodle.isNodeHovered(id) -> bool`

Check if the mouse cursor is inside a node's bounding box. Camera-aware.

### `doodle.isNodeClicked(id) -> bool`

Check if a node was left-clicked this frame. Camera-aware.

### `doodle.setPosition(id, x, y) -> bool`

Set absolute position of a node.

### `doodle.getPosition(id) -> (float, float)`

Get current position.

### `doodle.updateText(id, text) -> bool`

Update the inner text content.

### `doodle.showNode(id) / hideNode(id) -> bool`

Toggle node visibility.

### `doodle.removeNode(id) -> bool`

Permanently remove a node from the DOM tree.

---

## 7. Style Manipulation

### `doodle.setStyle(id, property, value) -> bool`

Set a CSS property at runtime.

```python
doodle.setStyle("box", "background-color", "#ff0000")
doodle.setStyle("box", "width", "200px")
doodle.setStyle("box", "border-radius", "12")
```

### `doodle.getStyle(id, property) -> str`

Read a CSS property value.

---

## 8. Audio & Synthesizer

### `doodle.playSound(path)`

Play a cached WAV/OGG sound file. Files are loaded once and reused.

```python
doodle.playSound("sfx/hit.wav")
```

### `doodle.playSynth(freq, duration, wave_type, attack, decay, sustain, release)`

Play a procedurally generated tone with ADSR envelope shaping.

```python
doodle.playSynth(
    freq=440.0,        # Frequency in Hz (A4)
    duration=0.15,     # Total duration in seconds
    wave_type=doodle.WAVE_TRIANGLE,
    attack=0.01,       # Seconds to reach peak volume
    decay=0.05,        # Seconds to settle to sustain level
    sustain=0.3,       # Sustain volume level (0.0 - 1.0)
    release=0.05       # Seconds to fade to silence
)
```

The synthesizer is polyphonic — multiple tones play simultaneously without blocking.

---

## 9. Visual Effects

### `doodle.spawnParticles(x, y, count, color_hex, speed, lifetime)`

Spawn a burst of particles at coordinates.

```python
doodle.spawnParticles(400, 300, 30, "#ff6b81", 4.5, 0.6)
```

| Parameter | Description |
|-----------|-------------|
| `x, y` | Spawn center position |
| `count` | Number of particles (from pre-allocated pool) |
| `color_hex` | Particle color as hex string |
| `speed` | Initial velocity magnitude |
| `lifetime` | Duration in seconds before fade-out |

### `doodle.shakeCamera(intensity, duration)`

Trigger a screen shake effect.

```python
doodle.shakeCamera(8.0, 0.3)  # Strong shake for 0.3 seconds
```

---

## 10. Camera Control

### `doodle.setCamera(target_x, target_y, offset_x, offset_y, zoom, rotation)`

Configure the 2D camera. Only affects nodes inside a `<view camera="true">` container.

```python
doodle.setCamera(
    target_x=400, target_y=300,   # World point the camera follows
    offset_x=400, offset_y=300,   # Screen-space center offset
    zoom=1.0,                     # Scale factor
    rotation=0.0                  # Rotation in degrees
)
```

---

## 11. Window Management

| Function | Description |
|----------|-------------|
| `setWindowTitle(title)` | Change the window title at runtime |
| `toggleFullscreen()` | Toggle between windowed and fullscreen |
| `getScreenSize()` | Returns `(width, height)` of the window |

---

## 12. Animation (Tweens)

### `doodle.animate(node_id, target_x, target_y, duration, ease)`

Smoothly animate a node's position.

```python
doodle.animate("paddle", target_x=350, target_y=500, duration=0.4, ease="quad_out")
```

| Parameter | Default | Description |
|-----------|---------|-------------|
| `target_x` | `None` | Target X position (omit to not animate X) |
| `target_y` | `None` | Target Y position |
| `duration` | `0.5` | Animation duration in seconds |
| `ease` | `"quad_out"` | Easing curve: `"linear"`, `"quad_in"`, `"quad_out"` |

---

## 13. Events

### `doodle.addEventListener(node_id, event_type, callback)`

Register a Python callback for node events.

```python
doodle.addEventListener("play-btn", "click", start_game)
doodle.addEventListener("menu-item", "hover", show_tooltip)
```

Supported event types: `"click"`, `"hover"`

### `doodle.setEventContext(dict)`

Provide a namespace for resolving inline `onclick="fn_name"` handlers from HTML.

```python
doodle.setEventContext({"restart": restart_game, "quit": quit_game})
```

---

## 14. Batch Processing

### `doodle.batchProcess(...) -> dict`

Combine multiple updates and collision checks into a single Python→C call for maximum performance.

```python
results = doodle.batchProcess(
    positions={
        "ball": (new_bx, new_by),
        "paddle": (px, py)
    },
    text_updates={
        "score": f"SCORE: {score}"
    },
    visibility_updates={
        "game-over": True
    },
    collisions=[
        ("ball", "paddle"),       # Direct ID collision
        ("ball", "bricks")        # Class group collision
    ]
)

# Results keyed by collision tuple:
if results[("ball", "paddle")]:     # True/False for direct collisions
    bounce()
hit = results[("ball", "bricks")]   # Node ID string or None for group collisions
```

---

## 15. Debugging & Introspection

### `doodle.getLineNumber(id) -> int`

Get the line number in `layout.html` where a node was defined.

### `doodle.getLayoutSize(id) -> (float, float)`

Get the computed `(width, height)` of a node's layout box.

### `doodle.isVisible(id) -> bool`

Check if a node is currently visible.

### `doodle.getText(id) -> str`

Get the current text content of a node.

### Runtime Console

Press **`~`** (tilde) during runtime to open the developer console. Type any Python expression to evaluate it live.

### `doodle.registerConsoleCallback(fn)`

Override the default console command handler.

```python
def my_handler(cmd: str) -> str:
    return eval(cmd)

doodle.registerConsoleCallback(my_handler)
```

---

## 16. Constants

### Waveform Types

| Constant | Value | Description |
|----------|-------|-------------|
| `WAVE_SINE` | `0` | Smooth sine wave |
| `WAVE_SQUARE` | `1` | Sharp square wave |
| `WAVE_TRIANGLE` | `2` | Triangular wave |
| `WAVE_SAWTOOTH` | `3` | Sawtooth ramp wave |
| `WAVE_NOISE` | `4` | White noise |
