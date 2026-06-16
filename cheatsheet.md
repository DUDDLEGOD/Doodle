# Doodle Engine Cheatsheet

Doodle is a high-performance, hybrid C-Python framework combining hardware-accelerated 2D graphics, dynamic XML layouts, CSS stylesheets, and a polyphonic audio synthesizer.

---

## 1. Markup Syntax (`layout.html`)

### Core Tags
* `<view>`: Container panel. Supports cameras and off-screen shader targets.
  * `<view camera="true">` maps a 2D camera boundary.
* `<text>`: Plain text rendering.
  * Supports placeholders like `SCORE: {{ score }}` for reactive state dictionary binding.
* `<image>`: Renders image textures. Needs `src` attribute.
* `<button>`: Focusable text button.
* `<circle>`: Draws solid circle shape primitives.
  * Attributes: `radius`, `color`.
* `<line>`: Draws vector line primitives.
  * Attributes: `x2`, `y2`, `thickness`, `color`.

### Event Attribute Hooks
Trigger Python callback routines directly:
* `onclick="callback_name"`: Fired when left mouse button is pressed inside bounds.
* `onhover="callback_name"`: Fired when mouse hovers over element.

---

## 2. Styling System (`styles.css`)

### Layout Constraints
* `width` / `height`: Can be set as fixed pixels (`100px`), percentages (`50%`), relative fits (`fit`), or grow containers (`grow`).
* `margin-left`, `margin-right`, `margin-top`, `margin-bottom`
* `padding-left`, `padding-right`, `padding-top`, `padding-bottom`

### Flexbox Rules
* `display: flex`
* `flex-direction`: `row` | `column`
* `justify-content`: `flex-start` | `flex-end` | `center` | `space-between` | `space-around`
* `align-items`: `flex-start` | `flex-end` | `center` | `stretch`
* `flex-grow`: Relative integer growth weights.

### Styles & Cosmetics
* `background-color`: Hex (`#ff0000`) or standard HTML color name.
* `text-color`: Font color.
* `font-size`: Point size.
* `font-path`: Path to TrueType font file (e.g. `assets/font.ttf`).
* `border-width` / `border-color` / `border-radius` (for rounded rectangles).
* `opacity`: Floating value from `0.0` to `1.0`.
* `shader-path`: Custom GLSL fragment shader file (e.g. `shaders/crt.fs`).

---

## 3. Python OOP APIs

### Initialization & State Bindings
```python
# Launch the engine with state-dictionary templates
state = {"score": 0, "lives": 3}
doodle.run(layout="layout.html", style="styles.css", state=state)
```

### OOP Node Proxies
```python
# Fetch reference
node = doodle.get_node("paddle")

# Read/write position coordinates
node.position = (350, 500)
node.x = 200
node.y = 500

# Write style values dynamically (translates snake_case to CSS dash-names)
node.style.background_color = "#00ffcc"
node.style.border_radius = "8"

# Write text
node.text = "NEW GAME OVER"

# Visibility
node.hide()
node.show()
```

### Easing Animations (Tweens)
```python
# Animate properties using custom easing curves
doodle.animate("paddle", target_x=350, target_y=500, duration=0.4, ease="quad_out")
```
* Supported Easing Curves: `linear`, `quad_in`, `quad_out`

---

## 4. Input & Event Polling

### Keyboard Input
* `doodle.is_key_down(key_code)`: Check if key is held.
* `doodle.is_key_pressed(key_code)`: Check if key was pressed in current frame.
  * Common Keys: `263` (Left Arrow), `262` (Right Arrow), `82` (R Key).

### Mouse Input
* `doodle.get_mouse_position()` -> returns `(x, y)` tuple.
* `doodle.get_mouse_x()` / `doodle.get_mouse_y()`
* `doodle.is_mouse_button_down(button)` / `doodle.is_mouse_button_pressed(button)`
* `doodle.set_mouse_cursor(cursor_type)`: Change cursor look (e.g. `4` for pointing pointer).

---

## 5. Procedural Synthesizer & Audio

Generate retro audio sounds procedurally using math waves:
```python
doodle.play_synth(freq, duration, type, attack, decay, sustain, release)
```
* **Waveforms:**
  * `doodle.WAVE_SINE` (`0`)
  * `doodle.WAVE_SQUARE` (`1`)
  * `doodle.WAVE_TRIANGLE` (`2`)
  * `doodle.WAVE_SAWTOOTH` (`3`)
  * `doodle.WAVE_NOISE` (`4`)
* **Envelope (ADSR):**
  * `attack` (seconds to peak volume)
  * `decay` (seconds to settle volume)
  * `sustain` (sustain level ratio, `0.0` to `1.0`)
  * `release` (seconds to fade out)

---

## 6. Juice and Effects

### Camera Shake
```python
# Screen shake camera mode
doodle.shake_camera(intensity=6.0, duration=0.2)
```

### Particle Emitter
```python
# Spawn real-time particle explosion bursts
doodle.spawn_particles(x, y, count=30, color="#ffffff", speed=4.5, lifetime=0.6)
```
