# Doodle Engine — Example Showcase Programs

A curated list of programs to build as `examples/`, organized by category. Each entry lists **what it demonstrates** from the engine feature set.

---

## 🎮 Games

### 1. Pong
> Classic 2-player (or vs AI) paddle game.

- **Features tested:** `setPosition`, collision detection (rect-rect), keyboard input (`isKeyDown`), `playSynth` for bounce sounds, reactive score text via `{{ templates }}`, `animate` for serve animation
- **Complexity:** ⭐ Easy — Great starter example

### 2. Snake
> Grid-based snake that grows when eating food.

- **Features tested:** Keyboard input (`isKeyPressed`), dynamic `show_node`/`hide_node` toggling grid cells, `updateText` for score, `playSynth` for eat/death sounds, `shakeCamera` on death
- **Complexity:** ⭐⭐ Easy-Medium

### 3. Flappy Bird Clone
> Tap to fly through pipe gaps.

- **Features tested:** Gravity physics in tick callback, `setPosition` every frame, collision detection (rect-rect), `removeNode` + dynamic obstacle management, `spawnParticles` on death, CSS background scrolling
- **Complexity:** ⭐⭐ Medium

### 4. Space Invaders
> Rows of aliens march down; player shoots.

- **Features tested:** Batch processing (`batchProcess`) for moving many aliens simultaneously, `getFirstCollision` with class groups (bullets vs `"aliens"`), `spawnParticles` on kill, `playSynth` for laser/explosion, `shakeCamera` on hit, visibility toggling
- **Complexity:** ⭐⭐⭐ Medium

### 5. Asteroids
> Ship rotates and shoots floating rocks.

- **Features tested:** CSS `rotation` style property, circle-circle and circle-rect collisions, `setCamera` for world tracking, `spawnParticles` for explosions, procedural `playSynth` for thrust/fire, `removeNode` for destroyed asteroids
- **Complexity:** ⭐⭐⭐ Medium

### 6. Platformer (Simple)
> Mario-style jump between platforms, collect coins.

- **Features tested:** Gravity + jump physics, `isKeyDown` for movement, rect-rect collision for ground detection, `hide_node` for collected coins, `animate` for coin bounce, camera scrolling with `setCamera`, particle effects, `{{ score }}` template
- **Complexity:** ⭐⭐⭐ Medium-Hard

---

## 🎨 Creative Tools

### 7. Pixel Art Editor
> Click on a grid of colored cells to paint pixel art.

- **Features tested:** `isNodeClicked` on a large grid of cells, `setStyle("background-color", ...)` to paint, color palette with `isNodeClicked`, hover highlighting with CSS `:hover` styles, `updateText` for status bar
- **Complexity:** ⭐⭐ Medium

### 8. Synthesizer & Recorder
> Piano keyboard that plays procedural tones; records and replays sequences.

- **Features tested:** `playSynth` with all waveforms (SINE, SQUARE, TRIANGLE, SAWTOOTH, NOISE), ADSR envelope parameters, `isNodeClicked` + `isKeyPressed` for piano keys, `setStyle` to highlight active keys, `updateText` for recording status, Python list for recording playback
- **Complexity:** ⭐⭐⭐ Medium

### 9. Drawing Canvas
> Freehand drawing app with brush sizes and colors.

- **Features tested:** `isMouseButtonDown` + `getMousePosition` for continuous stroke tracking, dynamically creating colored nodes or circles at mouse positions, color picker palette, `setStyle` for brush preview, clear button with `removeNode`
- **Complexity:** ⭐⭐⭐ Medium

---

## 📱 Apps & Utilities

### 10. Calculator
> Standard 4-function calculator with button grid.

- **Features tested:** `isNodeClicked` on button grid, `updateText` for display, CSS grid layout with `flex-direction: row`, hover styles, `playSynth` for button click feedback, clean layout/CSS separation
- **Complexity:** ⭐ Easy — Perfect for showcasing clean HTML/CSS layout

### 11. To-Do List
> Add, check off, and delete tasks.

- **Features tested:** `addEventListener` for click events, `updateText` to modify task labels, `setStyle` for strikethrough/dimming completed items, `hide_node`/`show_node` for filtering, inline `onclick` event handlers, `{{ templates }}` for task counts
- **Complexity:** ⭐⭐ Easy-Medium

### 12. Stopwatch / Timer
> Digital stopwatch with lap times.

- **Features tested:** `time.perf_counter()` in tick callback, `updateText` to display formatted time, `setStyle` for color changes (green/red), `addEventListener` for Start/Stop/Reset/Lap buttons, `playSynth` for alarm beep
- **Complexity:** ⭐ Easy

### 13. Weather Dashboard (Mock)
> Beautiful dashboard with weather cards, animated icons, temperature display.

- **Features tested:** Complex CSS layout (nested flex rows/columns, padding, margins, border-radius), `setStyle` for dynamic theming, `updateText` for data binding, CSS `font-family` custom fonts, rich visual design with gradients and rounded cards
- **Complexity:** ⭐⭐ Medium — Great for showing off layout/styling power

---

## 🧪 Tech Demos & Stress Tests

### 14. Particle Fireworks
> Click anywhere to spawn colorful particle bursts.

- **Features tested:** `isMouseButtonPressed` + `getMousePosition`, `spawnParticles` with varied colors/speeds/lifetimes, `playSynth` for bang effects, `setCamera` zoom effects, dark background styling
- **Complexity:** ⭐ Easy — Visually impressive demo

### 15. Dev Console Playground
> Interactive Python REPL inside the engine window.

- **Features tested:** Built-in developer console (`~` key), `registerConsoleCallback`, live node manipulation from console (typing `getNode("box").x += 50` at runtime), `updateText` for output log
- **Complexity:** ⭐ Easy — Shows off the runtime console

### 16. Hot Reload Showcase
> Edit layout.html and styles.css while the app is running and watch changes appear instantly.

- **Features tested:** Hot reload system (file watcher), CSS property changes, layout restructuring, position preservation across reloads
- **Complexity:** ⭐ Easy — Just needs good documentation and a tutorial

### 17. Collision Visualizer
> Drag shapes around and see collision detection in real-time with visual feedback.

- **Features tested:** `isMouseButtonDown` + `getMousePosition` for dragging, `checkCollision` for rect-rect / circle-rect / circle-circle, `setStyle("background-color", ...)` to flash red on collision, `spawnParticles` on impact, labels showing collision state via `updateText`
- **Complexity:** ⭐⭐ Easy-Medium

---

## 🏗️ Recommended Build Order

Build these in roughly this order — each one layers on complexity and tests new features:

| # | Example | New features exercised |
|---|---------|----------------------|
| 1 | **Calculator** | Layout, click events, text updates |
| 2 | **Stopwatch** | Tick callback timing, state management |
| 3 | **Pong** | Movement, keyboard input, collisions, synth |
| 4 | **Particle Fireworks** | Mouse input, particles, camera |
| 5 | **Pixel Art Editor** | Large grids, dynamic styling, hover |
| 6 | **Snake** | Visibility toggling, grid logic, camera shake |
| 7 | **Synthesizer** | All waveforms, ADSR, recording logic |
| 8 | **To-Do List** | Event listeners, inline handlers, templates |
| 9 | **Flappy Bird** | Physics, dynamic obstacles, node removal |
| 10 | **Space Invaders** | Batch processing, class group collisions |
| 11 | **Collision Visualizer** | Dragging, all collision shapes, visual debug |
| 12 | **Weather Dashboard** | Complex layout showcase, theming |
| 13 | **Platformer** | Camera scrolling, gravity, full game |
| 14 | **Asteroids** | Rotation, camera tracking, circles |
| 15 | **Drawing Canvas** | Continuous input, dynamic node creation |
| 16 | **Dev Console Playground** | Runtime REPL, live manipulation |
| 17 | **Hot Reload Showcase** | Tutorial/documentation piece |

> [!TIP]
> The first 5-6 examples would give you excellent **portfolio-ready coverage** of every major engine feature. The rest are stretch goals that test edge cases and stress performance.
