# Doodle Engine — Layout & Styling Guide

This document covers all supported HTML tags, attributes, CSS properties, and layout rules.

---

## HTML Tags

Doodle uses an HTML-like markup language parsed by a custom single-pass C parser. All elements require explicit closing tags.

### `<view>`

General-purpose container element. The fundamental building block for all layouts.

```html
<view id="sidebar" class="panel">
    <!-- child elements -->
</view>
```

**Special attributes:**

| Attribute | Description |
|-----------|-------------|
| `id` | Unique identifier for Python access |
| `class` | CSS class name for styling and collision group queries |
| `camera="true"` | Makes this container a 2D camera viewport (affected by `setCamera`) |
| `style` | Inline CSS properties (e.g., `style="width: 100px; background-color: red;"`) |
| `onclick` | Python function name to call on click |
| `onhover` | Python function name to call on hover |

---

### `<text>`

Renders text content. Supports reactive template variables.

```html
<text id="score" class="hud-text">SCORE: {{ score }}</text>
<text id="label">Static Text</text>
```

Template variables (`{{ var }}`) are automatically updated each frame from the `state` dictionary passed to `doodle.run()`.

---

### `<image>`

Renders a texture from file. Images are cached after first load.

```html
<image id="logo" src="assets/logo.png"></image>
<image id="bg" src="textures/background.jpg"></image>
```

| Attribute | Description |
|-----------|-------------|
| `src` | Path to image file (PNG, JPG, BMP, etc.) |

---

### `<button>`

Same as `<text>` but semantically indicates a clickable element.

```html
<button id="play-btn" onclick="start_game">PLAY</button>
```

---

### `<circle>`

Renders a filled circle shape.

```html
<circle id="coin" radius="12" color="#facc15"></circle>
<circle id="bullet" radius="4" color="#ff0000"></circle>
```

| Attribute | Description |
|-----------|-------------|
| `radius` | Circle radius in pixels |
| `color` | Fill color (hex or named) |

Circles use circle-aware collision detection when checked with `checkCollision()`.

---

### `<line>`

Renders a line segment.

```html
<line id="laser" x2="200" y2="150" thickness="3" color="#00ff00"></line>
```

| Attribute | Description |
|-----------|-------------|
| `x2` | End X coordinate (relative to start position) |
| `y2` | End Y coordinate |
| `thickness` | Line width in pixels |
| `color` | Line color |

---

### `<audio>`

Declares a background audio stream.

```html
<audio id="bgm" src="music/track.ogg" autoplay loop></audio>
<audio id="ambient" src="sfx/rain.wav" loop></audio>
```

| Attribute | Description |
|-----------|-------------|
| `src` | Path to audio file |
| `autoplay` | Start playing immediately |
| `loop` | Loop continuously |

---

## Common Attributes (All Tags)

| Attribute | Description | Example |
|-----------|-------------|---------|
| `id` | Unique node identifier | `id="player"` |
| `class` | CSS class (also used as collision group) | `class="brick"` |
| `style` | Inline CSS | `style="width: 50px; background-color: #f00;"` |
| `onclick` | Click handler function name | `onclick="shoot"` |
| `onhover` | Hover handler function name | `onhover="highlight"` |

---

## CSS Properties

Styles can be applied via `styles.css` selectors or inline `style` attributes.

### Selectors

Doodle supports three selector types:

```css
/* Tag selector — matches all <text> elements */
text {
    font-size: 16;
    color: #ffffff;
}

/* Class selector — matches class="brick" */
.brick {
    width: 80px;
    height: 30px;
    background-color: #f43f5e;
}

/* ID selector — matches id="player" */
#player {
    width: 100px;
    height: 20px;
    background-color: #00ffcc;
}

/* Hover pseudo-class — applied on mouse hover */
#player:hover {
    background-color: #33ffdd;
}
.brick:hover {
    border-width: 2;
    border-color: #ffffff;
}
```

---

### Sizing

| Property | Values | Description |
|----------|--------|-------------|
| `width` | `100px`, `50%`, `grow`, `fit` | Element width |
| `height` | `100px`, `50%`, `grow`, `fit` | Element height |

**Sizing modes:**
- **`Npx`** — Fixed pixel size (e.g., `width: 200px`)
- **`N%`** — Percentage of parent container (e.g., `width: 50%`)
- **`grow`** — Expand to fill remaining space
- **`fit` / `fit-content`** — Shrink to content size

---

### Flexbox Layout

| Property | Values | Default | Description |
|----------|--------|---------|-------------|
| `flex-direction` | `row`, `column` | `column` | Main axis direction |
| `justify-content` | `flex-start`, `flex-end`, `center`, `space-between`, `space-around` | `flex-start` | Distribution along main axis |
| `align-items` | `flex-start`, `flex-end`, `center`, `stretch` | `flex-start` | Alignment on cross axis |

```css
.toolbar {
    flex-direction: row;
    justify-content: space-between;
    align-items: center;
}
```

---

### Spacing

| Property | Example | Description |
|----------|---------|-------------|
| `padding` | `padding: 10` or `padding: 10 20` or `padding: 10 20 10 20` | Inner spacing (1, 2, or 4 values) |
| `padding-left` | `padding-left: 15` | Individual padding sides |
| `padding-right` | `padding-right: 15` | |
| `padding-top` | `padding-top: 15` | |
| `padding-bottom` | `padding-bottom: 15` | |
| `margin` | Same as padding | Outer spacing |
| `margin-left/right/top/bottom` | Individual sides | |

---

### Positioning

| Property | Values | Description |
|----------|--------|-------------|
| `position` | `relative`, `absolute` | Positioning mode |
| `left` | `100px` | X offset (for absolute positioning) |
| `top` | `50px` | Y offset (for absolute positioning) |

```css
#ball {
    position: absolute;
    left: 400px;
    top: 300px;
}
```

**Note:** Nodes positioned via Python (`setPosition` / `node.x = ...`) automatically get absolute positioning.

---

### Visual Styles

| Property | Values | Description |
|----------|--------|-------------|
| `background-color` | `#ff5733`, `red`, `rgba(255,0,0,0.5)` | Fill color |
| `color` / `text-color` | Same as above | Text color |
| `font-size` | `24` | Text size in points |
| `font-family` | `"assets/font.ttf"` | Custom TrueType font path |
| `text-align` | `left`, `center`, `right` | Text alignment |
| `border-radius` | `8` | Rounded corner radius |
| `border-width` | `2` | Border thickness |
| `border-color` | `#ffffff` | Border color |
| `opacity` | `0.0` to `1.0` | Transparency |
| `display` | `flex`, `none` | Visibility (`none` hides the element) |
| `z-index` | Integer | Draw order (higher = drawn on top) |
| `rotation` | `45` | Rotation in degrees |
| `tint` | `#ff0000` | Tint overlay color (for images) |

---

### Shaders

Apply custom GLSL fragment shaders to any `<view>`:

```css
#game-area {
    shader: "shaders/crt.fs";
}
```

The shader receives the rendered contents of the view as a texture. Useful for post-processing effects like CRT scanlines, bloom, vignette, etc.

---

### Colors

Doodle supports multiple color formats:

| Format | Example |
|--------|---------|
| Hex (6-digit) | `#ff5733` |
| Hex (8-digit with alpha) | `#ff573380` |
| Named colors | `red`, `blue`, `white`, `black`, `green`, `yellow`, `gray`, `orange`, `purple`, `cyan`, `magenta`, `pink` |
| RGB function | `rgb(255, 87, 51)` |
| RGBA function | `rgba(255, 87, 51, 0.5)` |

---

## Layout Examples

### Full-Screen Column Layout
```css
#app {
    flex-direction: column;
    width: 100%;
    height: 100%;
}
```

### HUD Bar
```css
#hud {
    flex-direction: row;
    justify-content: space-between;
    width: 100%;
    height: 50px;
    padding: 10 20;
    background-color: #111111;
}
```

### Centered Content
```css
#container {
    flex-direction: column;
    justify-content: center;
    align-items: center;
    width: 100%;
    height: 100%;
}
```

### Grid of Items (Rows of Cards)
```html
<view class="grid">
    <view class="row">
        <view class="card"></view>
        <view class="card"></view>
        <view class="card"></view>
    </view>
    <view class="row">
        <view class="card"></view>
        <view class="card"></view>
        <view class="card"></view>
    </view>
</view>
```

```css
.grid {
    flex-direction: column;
    width: 100%;
}
.row {
    flex-direction: row;
    justify-content: space-around;
    margin-bottom: 10;
}
.card {
    width: 100px;
    height: 60px;
    background-color: #2a2a4a;
    border-radius: 8;
}
.card:hover {
    background-color: #3a3a6a;
}
```
