# Doodle Engine Cheatsheet v1.1.0

Quick reference for all markup, styling, and Python APIs.

---

## Layout Tags

| Tag | Purpose | Key Attributes |
|-----|---------|----------------|
| `<view>` | Container | `camera="true"` |
| `<text>` | Text node | `{{ var }}` templates |
| `<image>` | Texture | `src="path.png"` |
| `<button>` | Clickable text | `onclick="fn"` |
| `<circle>` | Circle shape | `radius`, `color` |
| `<line>` | Line segment | `x2`, `y2`, `thickness`, `color` |
| `<audio>` | Music stream | `src`, `autoplay`, `loop` |

**Common:** `id`, `class`, `style`, `onclick`, `onhover`

---

## CSS Properties

| Property | Example Values |
|----------|---------------|
| `width` / `height` | `100px`, `50%`, `grow`, `fit` |
| `flex-direction` | `row`, `column` |
| `justify-content` | `flex-start`, `center`, `space-between`, `space-around` |
| `align-items` | `flex-start`, `center`, `stretch` |
| `padding` | `10`, `10 20`, `10 20 10 20` |
| `margin` | Same as padding |
| `position` | `relative`, `absolute` |
| `left` / `top` | `100px` |
| `background-color` | `#ff5733`, `red`, `rgba(0,0,0,0.5)` |
| `color` | Same as bg-color |
| `font-size` | `24` |
| `font-family` | `"fonts/custom.ttf"` |
| `text-align` | `left`, `center`, `right` |
| `border-radius` | `8` |
| `border-width` | `2` |
| `border-color` | `#fff` |
| `opacity` | `0.0` – `1.0` |
| `display` | `flex`, `none` |
| `z-index` | Integer |
| `rotation` | Degrees |
| `shader` | `"shaders/crt.fs"` |
| `tint` | `#ff0000` |

**Selectors:** `tag { }`, `.class { }`, `#id { }`, `#id:hover { }`

---

## Python API (camelCase + snake_case)

### Lifecycle
```python
doodle.run(layout, style, width, height, title, state)
doodle.registerTickCallback(fn)
```

### Nodes (OOP)
```python
node = doodle.getNode("id")
node.x += 10; node.y = 200; node.position = (x, y)
node.width = 80; node.w; node.height; node.h
node.text = "Hello"; node.visible = False
node.show(); node.hide()
node.style.background_color = "#f00"
```

### Input
```python
doodle.isKeyDown(code); doodle.isKeyPressed(code)
doodle.getMousePosition(); doodle.getMouseX(); doodle.getMouseY()
doodle.isMouseButtonDown(btn); doodle.isMouseButtonPressed(btn)
doodle.getMouseWheelMove(); doodle.setMouseCursor(type)
doodle.isNodeClicked("id"); doodle.isNodeHovered("id")
```

### Collision
```python
doodle.checkCollision("a", "b")           # bool
doodle.getFirstCollision("ball", "bricks")  # str | None
```

### Position & DOM
```python
doodle.setPosition("id", x, y); doodle.getPosition("id")
doodle.updateText("id", "text"); doodle.removeNode("id")
doodle.showNode("id"); doodle.hideNode("id")
doodle.setStyle("id", "prop", "val"); doodle.getStyle("id", "prop")
```

### Audio
```python
doodle.playSound("sfx/hit.wav")
doodle.playSynth(frequency, duration, wave_type=1, attack=0.01, decay=0.05, sustain=0.5, release=0.05, frequency_slide=0.0, vibrato_speed=0.0, vibrato_depth=0.0, tremolo_speed=0.0, tremolo_depth=0.0, filter_cutoff=0.0, pan=0.0)
# WAVE_SINE=0 WAVE_SQUARE=1 WAVE_TRIANGLE=2 WAVE_SAWTOOTH=3 WAVE_NOISE=4
```

### Effects
```python
doodle.spawnParticles(x, y, count, "#color", speed, lifetime)
doodle.shakeCamera(intensity, duration)
doodle.animate("id", target_x=100, target_y=200, duration=0.5, ease="quad_out")
```

### Camera
```python
doodle.setCamera(tx, ty, ox, oy, zoom, rot)
```

### Window
```python
doodle.setWindowTitle("title"); doodle.toggleFullscreen(); doodle.getScreenSize()
```

### Events
```python
doodle.addEventListener("id", "click", fn)
doodle.setEventContext({"fn_name": fn})
```

### Batch
```python
results = doodle.batchProcess(
    positions={"id": (x,y)},
    text_updates={"id": "text"},
    visibility_updates={"id": True},
    collisions=[("a","b")]
)
```

### Debug
```python
doodle.getLineNumber("id"); doodle.getLayoutSize("id")
doodle.isVisible("id"); doodle.getText("id")
# Press ~ for runtime console
```

---

## Key Codes

| Key | Code | Key | Code | Key | Code |
|-----|------|-----|------|-----|------|
| ← | 263 | A | 65 | Space | 32 |
| → | 262 | D | 68 | Enter | 257 |
| ↑ | 265 | W | 87 | Esc | 256 |
| ↓ | 264 | S | 83 | R | 82 |
