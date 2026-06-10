# Doodle UI Engine Specification & Implementation Guide

This document contains the complete software requirements, file architecture, API definitions, and implementation pathways for **Doodle**, a lightweight, hardware-accelerated UI and 2D game engine.

Doodle pairs a high-performance C core (powered by Raylib) with an elegant Python scripting layer via the CPython API. It completely bypasses browser engines (like Chromium) by implementing a custom, single-pass HTML/CSS parser and layout engine that maps web-standard layout rules directly to native GPU drawing commands and collision systems.

---

## 1. System Architecture & File Structure

The project compiles into a single, high-performance native Python extension module (`myui` or `doodle`). It is broken down into three distinct C modules and one Python build script to maintain strict separation of concerns.

```text
doodle/
├── dutils.h         # Utility types and internal math helpers
├── dutils.c         # Math implementations, hex parsing, bounding-box helpers
├── mparser.h        # DOM tree definitions and CSS style structs
├── mparser.c        # Single-pass HTML string scanner & CSS registry mapper
├── expose_raylib.c  # CPython extension entry point, window lifecycle, & main loop
└── setup.py         # Setuptools compilation script

```

### Compilation Blueprint (`setup.py`)

This script uses standard Python tooling to compile the independent C modules and link them against the local installation of Raylib.

```python
from setuptools import setup, Extension

doodle_module = Extension(
    'doodle',
    sources=['expose_raylib.c', 'mparser.c', 'dutils.c'],
    include_dirs=['.'],          # Local header search path
    libraries=['raylib'],        # Links against native raylib binary
    extra_compile_args=['-std=c99']
)

setup(
    name='doodle',
    version='1.0',
    description='Hardware-accelerated DOM-based UI & 2D Game Engine',
    ext_modules=[doodle_module],
    install_requires=['raylib']   # Pulls raylib-python-cffi for the python layer
)

```

---

## 2. Component Specifications

### 2.1. Utilities Engine (`dutils.c` & `dutils.h`)

Responsible for abstracting non-UI computations and converting high-level configuration formats to native structures.

* **Color Translation:** Evaluates string hex formats (`#FFFFFF`, `#FF5733`) or alpha formats (`rgba(0,0,0,0.5)`) and converts them directly into Raylib `Color` structures (`unsigned char r, g, b, a`).
* **Unit Normalization:** Strips text designations (e.g., converting text-based padding numbers like `"15px"` or `"20"`) into floating-point structures.
* **Vector Geometry:** Math wrappers for basic bounding boxes (`Rectangle` conversions) to serve the coordinate pipeline.

### 2.2. Parser Engine (`mparser.c` & `mparser.h`)

Instead of an abstract key-value setup, this module tokenizes traditional, structured syntax. It outputs an in-memory representation of layout frames using an n-ary DOM node tree.

#### The Node Architecture

```c
typedef enum { NODE_VIEW, NODE_TEXT, NODE_IMAGE, NODE_BUTTON, NODE_AUDIO } NodeType;
typedef enum { DIR_ROW, DIR_COLUMN } FlexDirection;

typedef struct {
    float x, y, width, height;
} LayoutBox;

typedef struct {
    Color bg_color;
    Color border_color;
    float border_width;
    float border_radius;
    float opacity;
    FlexDirection flex_direction;
    char font_path[256];
    float font_size;
    Color text_color;
} StyleProps;

typedef struct UINode {
    char id[64];
    NodeType type;
    char text_content[512];
    char asset_path[256];

    StyleProps style;       // Populated by CSS mapper
    LayoutBox layout;       // Populated by Flexbox calculator

    struct UINode* parent;
    struct UINode* children[128];
    int child_count;
} UINode;

```

#### The Functional Registry Pattern

To scale the engine without complex conditions, the CSS processing engine maps web style properties to their handling targets via a static lookup registry:

```c
typedef void (*CSSPropertyHandler)(UINode* node, const char* value);

typedef struct {
    const char* property_name;
    CSSPropertyHandler handler;
} CSSMap;

// Example Registry Hook
CSSMap css_registry[] = {
    {"flex-direction", handle_flex_direction},
    {"border-radius", handle_border_radius},
    {"background-color", handle_bg_color},
    {"width", handle_width},
    {"height", handle_height}
};

```

### 2.3. Bridge & Rendering Engine (`expose_raylib.c`)

Manages the OS window hook, hooks into the Python lifecycle using the CPython extension interface, manages the 60 FPS update cycle, and coordinates native drawing.

* **Initialization & Context Hooks:** Boots up the graphics layout and coordinates the audio context initialization (`InitAudioDevice()`).
* **Dynamic Resource File Watcher:** Polls file system structures for file update events (`st_mtime`). If changes occur, it flushes the current DOM tree memory context safely (`free()`) and invokes the parsing workflow to rebuild the layout instantly.
* **Native Drawing Traversal:** Iterates over the active tree structures and runs hardware-accelerated Raylib instructions (`DrawRectangleRounded`, `DrawTextEx`, `DrawTexture`) targeting the absolute rendering coordinates computed during layout processing.

---

## 3. High-Fidelity Interactive Layout & Game Mechanics

### 3.1. DOM-Based Entity Positioning & Real-Time Modification

Doodle eliminates the requirement of tracking rendering positions inside script modules. Entities (such as paddles, blocks, or characters) are defined as components inside the core DOM document structure and positioned via absolute modifications.

#### HTML Architecture (`layout.html`)

```html
<view class="stage">
    <view class="hud">
        <text id="score-text">SCORE: 0000</text>
        <text id="lives-text">LIVES: 3</text>
    </view>

    <view class="game-space" id="arena">
        <view id="paddle" class="paddle-entity"></view>
        <view id="ball" class="ball-entity"></view>
        <view id="bricks-container" class="grid-layout"></view>
    </view>
</view>

```

#### CSS Layout Declarations (`styles.css`)

```css
.stage {
    flex-direction: column;
    width: 100%;
    height: 100%;
    background-color: #080808;
}

.hud {
    flex-direction: row;
    justify-content: space-between;
    padding: 15px 30px;
    background-color: #111111;
}

.game-space {
    width: 100%;
    height: 100%;
    position: relative; /* Context bounding box for absolutely positioned inner items */
}

.paddle-entity {
    width: 100px;
    height: 20px;
    background-color: #c0c0c0;
    border-radius: 4px;
}

.ball-entity {
    width: 20px;
    height: 20px;
    background-color: #ffffff;
    border-radius: 10px; /* Makes the square bounding box look like a circle */
}

```

---

## 4. Native Engine Features

### 4.1. Hardware-Accelerated Collision Engine

Doodle exposes Raylib’s highly optimized spatial overlap functions (`CheckCollisionRecs`, `CheckCollisionCircleRec`) through DOM object IDs. Python script structures delegate the actual intersection computations to the native C Layer.

```c
// Internal C implementation exposed via CPython array mapping
static PyObject* doodle_check_collision(PyObject* self, PyObject* args) {
    const char* id_a;
    const char* id_b;
    if (!PyArg_ParseTuple(args, "ss", &id_a, &id_b)) return NULL;

    UINode* node_a = find_node_by_id(root, id_a);
    UINode* node_b = find_node_by_id(root, id_b);

    if (!node_a || !node_b) Py_RETURN_FALSE;

    // Convert computed layout positions to Raylib structures
    Rectangle rec_a = {node_a->layout.x, node_a->layout.y, node_a->layout.width, node_a->layout.height};
    Rectangle rec_b = {node_b->layout.x, node_b->layout.y, node_b->layout.width, node_b->layout.height};

    if (CheckCollisionRecs(rec_a, rec_b)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

```

### 4.2. Embedded Native Audio Architecture

Audio streams and static wave effects load directly into memory contexts controlled by the C engine, ensuring that sound playback does not block the Python script interpreter loop.

* **Declarative Hooks:** Markup parameters parse standard definitions to create background loops (`<audio id="bgm" src="music.wav" autoplay loop>`). The main loop execution process updates these asset arrays transparently (`UpdateMusicStream()`).
* **Imperative Event Playback Pipeline:** Fast sound-trigger endpoints are exposed to script logic, caching and replaying sound buffers with minimum latency.

---

## 5. Script Application Design Blueprint (`main.py`)

This implementation demonstrates how clean and straightforward the final developer experience becomes. Python handles state variations and operational responses, while Doodle manages rendering, alignment layouts, collisions, and sound streaming underneath.

```python
import doodle

# Ball Vector Configurations
ball_dx = 6.0
ball_dy = -6.0
score = 0
lives = 3

def game_loop_tick():
    global ball_dx, ball_dy, score, lives

    # 1. Access Current Transform States directly from the DOM
    bx, by = doodle.get_position("ball")
    px, py = doodle.get_position("paddle")

    # 2. Update Node Spatial Positioning
    new_bx = bx + ball_dx
    new_by = by + ball_dy
    doodle.set_position("ball", new_bx, new_by)

    # 3. Request Low-Level Native Collision Resolution
    if doodle.check_collision("ball", "paddle"):
        ball_dy *= -1.0
        doodle.play_sound("assets/paddle_hit.wav")

    # 4. Filter Component Interactions via Target Groups
    hit_brick = doodle.get_first_collision("ball", group="bricks")
    if hit_brick:
        ball_dy *= -1.0
        doodle.remove_node(hit_brick) # Mutates the layout tree natively
        score += 100
        doodle.update_text("score-text", f"SCORE: {score:04d}")
        doodle.play_sound("assets/brick_pop.wav")

    # 5. Screen boundary checks
    if new_bx <= 0 or new_bx >= 800 - 20:
        ball_dx *= -1.0
    if new_by <= 0:
        ball_dy *= -1.0
    if new_by >= 600:
        lives -= 1
        doodle.update_text("lives-text", f"LIVES: {lives}")
        if lives <= 0:
            doodle.show_node("game-over-screen")
        else:
            doodle.set_position("ball", 400, 300) # Reset position

# Connect Python Game Logic to the Doodle Lifecycle Window
doodle.register_tick_callback(game_loop_tick)

# Boot up the native loop environment
doodle.run(
    layout="layout.html",
    style="styles.css",
    width=800,
    height=600,
    title="Doodle Breakout Engine"
)

```

---

## 6. Implementation Checklist for Antigravity

When translating this configuration design into code execution, implement the engine modules in this exact chronological order:

1. **Phase 1 (`dutils`):** Write string splitting algorithms and implement hex color parsing mechanisms. Ensure these functions are pure C and free of Python dependencies.
2. **Phase 2 (`mparser`):** Implement the HTML tree token splitting logic. Create generic tree traversal nodes that treat unrecognized tags as default structural components to achieve wide layout compatibility. Next, build the dictionary function mapper for CSS processing rules.
3. **Phase 3 (`expose_raylib`):** Setup the boilerplate configuration for Python interaction parameters (`PyMethodDef`). Add the core window activation routine (`InitWindow`), attach the timestamp tracking loop to create real-time layout updates, and integrate `InitAudioDevice()`.
4. **Phase 4 (Layout Math):** Implement a simple layout calculator that traverses the tree to compute box boundaries based on margin, padding, width, and height definitions.
5. **Phase 5 (Script Binding API):** Bind `doodle_check_collision` and `doodle_set_position` routines into the CPython interface table to enable direct DOM transformations from the Python script layer.


