import doodle

# ── Game State ──
state = {
    "score": 0,
    "lives": 3,
    "paddle_glitch": 0.0,
    "destroyed_bricks": []
}

BALL_SPEED = 5.0
PADDLE_SPEED = 9.0
PADDLE_Y = 430
BALL_START_X = 390
BALL_START_Y = 250

ball_dx = BALL_SPEED
ball_dy = -BALL_SPEED
game_over = False
victory = False
active_bricks = 32

# Coordinate offsets to map local coordinates to absolute screen coordinates.
# Arena is positioned at (0, 100) with padding-left: 15px and padding-top: 25px.
# So the inner arena content starts at absolute (15, 125).
OFFSET_X = 15
OFFSET_Y = 125

# Brick row colors for particles
ROW_COLORS = {1: "#ff6b81", 2: "#ffa502", 3: "#ffd43b", 4: "#51cf66"}

# ── Node References (lazy init) ──
paddle = None
ball = None
game_over_screen = None
victory_screen = None

def restart_game():
    global ball_dx, ball_dy, game_over, victory, active_bricks
    state["score"] = 0
    state["lives"] = 3
    state["paddle_glitch"] = 0.0
    ball_dx = BALL_SPEED
    ball_dy = -BALL_SPEED
    game_over = False
    victory = False
    active_bricks = 32

    paddle.position = (350, PADDLE_Y)
    ball.position = (BALL_START_X, BALL_START_Y)
    doodle.setStyle("paddle", "shader", "")
    doodle.setStyle("paddle", "rotation", "0.0")

    game_over_screen.hide()
    victory_screen.hide()

    # Play restart sound
    doodle.playSynth(523.0, 0.1, doodle.WAVE_TRIANGLE, 0.01, 0.02, 0.3, 0.02)

    state["destroyed_bricks"] = []
    for r in range(1, 5):
        for c in range(1, 9):
            brick_id = f"b{r}{c}"
            brick = doodle.getNode(brick_id)
            if brick:
                brick.show()
                doodle.setStyle(brick_id, "shader", "")
                doodle.setStyle(brick_id, "background-color", ROW_COLORS[r])

def tick():
    global ball_dx, ball_dy, game_over, victory, active_bricks
    global paddle, ball, game_over_screen, victory_screen

    # Lazy init nodes (first tick)
    if paddle is None:
        paddle = doodle.getNode("paddle")
        ball = doodle.getNode("ball")
        game_over_screen = doodle.getNode("game-over")
        victory_screen = doodle.getNode("victory-screen")


    if game_over or victory:
        doodle.setMouseCursor(4 if doodle.isNodeHovered("restart-btn") or doodle.isNodeHovered("restart-btn-2") else 0)
        return

    # ── Paddle Movement ──
    px, py = paddle.position

    if doodle.isKeyDown(263):  # LEFT
        px -= PADDLE_SPEED
    if doodle.isKeyDown(262):  # RIGHT
        px += PADDLE_SPEED

    mx = doodle.getMouseX()
    if 0 < mx < 800:
        px = mx - 55.0

    px = max(0.0, min(690.0, px))

    # ── Ball Movement ──
    bx, by = ball.position
    new_bx = bx + ball_dx
    new_by = by + ball_dy

    # ── Wall bounces with effects (offsetting spawned particles to align with drawing camera) ──
    if new_bx <= 10:
        ball_dx = -ball_dx
        new_bx = 10
        doodle.playSynth(220.0, 0.04, doodle.WAVE_SINE, 0.002, 0.01, 0.2, 0.01)
        doodle.spawnParticles(new_bx + OFFSET_X, new_by + OFFSET_Y, 8, "#475569", 2.0, 0.3)

    if new_bx >= 780:
        ball_dx = -ball_dx
        new_bx = 780
        doodle.playSynth(220.0, 0.04, doodle.WAVE_SINE, 0.002, 0.01, 0.2, 0.01)
        doodle.spawnParticles(new_bx + OFFSET_X, new_by + OFFSET_Y, 8, "#475569", 2.0, 0.3)

    if new_by <= 5:
        ball_dy = -ball_dy
        new_by = 5
        doodle.playSynth(196.0, 0.04, doodle.WAVE_SINE, 0.002, 0.01, 0.2, 0.01)
        doodle.spawnParticles(new_bx + OFFSET_X, new_by + OFFSET_Y, 8, "#475569", 2.0, 0.3)

    # ── Batch update positions ──
    doodle.batchProcess(
        positions={
            "paddle": (px, py),
            "ball": (new_bx, new_by),
        }
    )

    # ── Paddle Collision ──
    if doodle.checkCollision("ball", "paddle"):
        if ball_dy > 0:
            ball_dy = -ball_dy
            hit_factor = (new_bx - (px + 55)) / 55.0
            ball_dx = hit_factor * 6.0
            # Bright ping sound
            doodle.playSynth(440.0, 0.08, doodle.WAVE_TRIANGLE, 0.005, 0.02, 0.4, 0.02)
            # Cyan spray of particles (properly offset to hit point)
            doodle.spawnParticles(new_bx + OFFSET_X, new_by + 8 + OFFSET_Y, 25, "#74b9ff", 4.0, 0.4)
            doodle.shakeCamera(3.0, 0.1)

    # ── Brick Collisions ──
    for r in range(1, 5):
        for c in range(1, 9):
            brick_id = f"b{r}{c}"
            if brick_id not in state.get("destroyed_bricks", []) and doodle.checkCollision("ball", brick_id):
                brick = doodle.getNode(brick_id)
                if brick:
                    ball_dy = -ball_dy

                    # Row-dependent pitch — higher rows = higher pitch
                    pitch = 260.0 + (5 - r) * 80.0
                    doodle.playSynth(pitch, 0.08, doodle.WAVE_SQUARE, 0.005, 0.02, 0.35, 0.02)

                    # Colored particle burst matching the brick row
                    color = ROW_COLORS.get(r, "#ffffff")
                    doodle.spawnParticles(new_bx + OFFSET_X, new_by + OFFSET_Y, 35, color, 5.0, 0.5)
                    # Extra white sparkle
                    doodle.spawnParticles(new_bx + OFFSET_X, new_by + OFFSET_Y, 10, "#ffffff", 3.0, 0.3)
                    doodle.shakeCamera(4.0, 0.12)
                    if brick.style.shader == "shaders/hit.fs":
                        # Second hit: make it transparent to keep its layout position without shifting other bricks
                        doodle.setStyle(brick_id, "background-color", "transparent")
                        doodle.setStyle(brick_id, "shader", "")
                        state.setdefault("destroyed_bricks", []).append(brick_id)
                        state["score"] += (5 - r) * 50  # Top rows worth more
                        active_bricks -= 1
                    else:
                        doodle.setStyle(brick_id, "shader", "shaders/hit.fs")

                    if active_bricks <= 0:
                        victory = True
                        victory_screen.show()
                        # Victory fanfare
                        doodle.playSynth(523.0, 0.2, doodle.WAVE_TRIANGLE, 0.01, 0.05, 0.5, 0.1)
                        doodle.spawnParticles(400 + OFFSET_X, 250 + OFFSET_Y, 80, "#ffd43b", 6.0, 1.0)
                        doodle.spawnParticles(400 + OFFSET_X, 250 + OFFSET_Y, 50, "#ff6b81", 5.0, 0.8)
                        doodle.shakeCamera(8.0, 0.3)
                break

    # ── Bottom boundary (Lose Life) ──
    if new_by >= 460:
        # Deep thud sound
        doodle.playSynth(80.0, 0.4, doodle.WAVE_SAWTOOTH, 0.01, 0.15, 0.1, 0.15)
        # Red danger particles
        doodle.spawnParticles(new_bx + OFFSET_X, 460 + OFFSET_Y, 40, "#ff6b81", 4.0, 0.6)
        doodle.shakeCamera(8.0, 0.25)
        state["lives"] -= 1

        if state["lives"] <= 0:
            game_over = True
            game_over_screen.show()
            # Game over sound
            doodle.playSynth(65.0, 0.6, doodle.WAVE_SAWTOOTH, 0.01, 0.2, 0.05, 0.3)
            doodle.spawnParticles(400 + OFFSET_X, 300 + OFFSET_Y, 60, "#ff6b81", 5.0, 1.0)
        else:
            paddle.position = (350, PADDLE_Y)
            ball.position = (BALL_START_X, BALL_START_Y)
            ball_dx = BALL_SPEED
            ball_dy = -BALL_SPEED

            doodle.batchProcess(
                positions={
                    "paddle": (350, PADDLE_Y),
                    "ball": (BALL_START_X, BALL_START_Y),
                }
            )

# ── Setup ──
doodle.registerTickCallback(tick)
doodle.setEventContext({"restart": restart_game})

doodle.run(
    layout="layout.html",
    style="styles.css",
    width=800,
    height=600,
    title="Breakout - CRT Edition",
    state=state,
)
