# Unused Features Reference: Doodle Engine

This document tracks all features built into the Doodle layout parser and native binding engine that are currently unused in the Breakout game implementation.

---

## 1. Unused Native C & Raylib Bindings
These C bindings are exposed to Python in the compiled extension but are not called in `main.py`:
* `doodle.is_mouse_button_down(button)`: Checks if mouse button is held.
* `doodle.is_mouse_button_pressed(button)`: Checks if mouse button was clicked.
* `doodle.get_mouse_wheel_move()`: Returns scroll wheel offset value.
* `doodle.get_mouse_position()`: Returns mouse position tuple `(x, y)`.
* `doodle.get_mouse_y()`: Retrieves only the mouse Y coordinate.
* `doodle.toggle_fullscreen()`: Toggles hardware window fullscreen mode.
* `doodle.get_screen_size()`: Returns current desktop rendering bounds.
* `doodle.set_window_title(title)`: Modifies application window header.
* `doodle.play_sound(path)`: Plays a `.wav` file (fully replaced by the procedural `play_synth` engine).

---

## 2. Unused HTML Tags
These element types are supported by the XML parser (`mparser.c`) but are not utilized in `layout.html`:
* `<image>`: Used for rendering texture bitmaps (e.g. `<image src="logo.png" />`).
* `<audio>`: Declares background loop streams (e.g. `<audio src="music.ogg" loop="true" />`).
* `<line>`: Renders custom segment paths (e.g. `<line x2="100" y2="100" thickness="3" color="#ff0000" />`).

---

## 3. Unused CSS Styles & Attributes
These styling parameters are registered in the CSS parser but are not declared in `styles.css`:
* `flex-direction`: Direction of flexbox item distribution (`row` | `column`).
* `justify-content`: Space alignment along the main flex axis (`center`, `space-between`, etc.).
* `align-items`: Aligning items perpendicular to layout direction (`center`, `stretch`, etc.).
* `font-path`: Custom font loader (.ttf file binding).
* `opacity`: Layer transparency level (float range `0.0` to `1.0`).
