import os
import sys

import doodle

# Game state dictionary (Passed to doodle.run for reactive templating!)
state = {
    "score": 0,
    "lives": 3
}

ball_dx = 5.0
ball_dy = -5.0
game_over = False

# OOP Node wrappers
ball = doodle.get_node("ball")
paddle = doodle.get_node("paddle")
game_over_screen = doodle.get_node("game-over-screen")

# Fast static lookups for brick configuration
_PITCH_MAP = {"b1": 783.99, "b2": 659.25, "b3": 587.33, "b4": 523.25}
_COLOR_MAP = {"b1": "#f43f5e", "b2": "#fb923c", "b3": "#facc15", "b4": "#4ade80"}

def restart_game():
    global ball_dx, ball_dy, game_over
    try:
        os.utime("layout.html", None) # Trigger hot reload reset
    except Exception:
        pass

    state["score"] = 0
    state["lives"] = 3
    game_over = False

    game_over_screen.hide()

    # Place entities
    ball.position = (390, 350)
    paddle.position = (350, 500)

    ball_dx = 5.0
    ball_dy = -5.0

    # Play procedural restart sound tone
    doodle.play_synth(440.0, 0.15, doodle.WAVE_TRIANGLE, 0.01, 0.05, 0.3, 0.05)

    # Trigger a smooth entrance animation for the paddle
    paddle.y = 580
    doodle.animate("paddle", target_y=500, duration=0.4, ease="quad_out")

def game_loop_tick():
    global ball_dx, ball_dy, game_over

    # Global Reset Key (R)
    if doodle.is_key_pressed(82): # KEY_R = 82
        restart_game()
        doodle.shake_camera(10.0, 0.4)

    if game_over:
        doodle.set_mouse_cursor(4 if doodle.is_node_hovered("restart-btn") else 0)
        return

    # 1. Update Paddle Positioning
    px, py = paddle.position

    # Keyboard movement
    if doodle.is_key_down(263): # Left
        px -= 9.0
    if doodle.is_key_down(262): # Right
        px += 9.0

    # Mouse movement
    mx = doodle.get_mouse_x()
    if 0 < mx < 800:
        px = mx - 50.0

    # Paddle boundary checks
    if px < 0:
        px = 0.0
    elif px > 700.0:  # 800 - 100
        px = 700.0
    # 2. Calculate Ball Position
    bx, by = ball.position
    new_bx = bx + ball_dx
    new_by = by + ball_dy

    # 3. Batch process positions and collisions!
    res = doodle.batch_process(
        positions={
            "paddle": (px, py),
            "ball": (new_bx, new_by)
        },
        collisions=[
            ("ball", "paddle"),
            ("ball", "bricks")
        ]
    )

    # Dynamic camera offset tracking
    doodle.set_camera(400.0, 300.0, 400.0 + ball_dx * 1.5, 300.0 + ball_dy * 1.5, 1.0, 0.0)

    # 3. Collision with Paddle
    if res.get(("ball", "paddle"), False):
        ball_dx = ((new_bx + 8) - (px + 50)) * 0.16
        ball_dy = -abs(ball_dy)

        # Procedural Sound Synth (Paddle Hit)
        doodle.play_synth(293.66, 0.08, doodle.WAVE_SQUARE, 0.005, 0.02, 0.4, 0.02)
        doodle.shake_camera(4.0, 0.15)
        doodle.spawn_particles(new_bx + 8, new_by + 8, 8, "#00ffcc", 2.0, 0.3)

    # 4. Collision with Bricks
    hit_brick = res.get(("ball", "bricks"), None)
    if hit_brick:
        ball_dy = -ball_dy
        doodle.remove_node(hit_brick)
        state["score"] += 100

        prefix = hit_brick[:2]
        pitch = _PITCH_MAP.get(prefix, 783.99)
        doodle.play_synth(pitch, 0.06, doodle.WAVE_TRIANGLE, 0.002, 0.01, 0.2, 0.01)
        doodle.shake_camera(6.0, 0.2)

        p_color = _COLOR_MAP.get(prefix, "#ffffff")
        doodle.spawn_particles(new_bx + 8, new_by + 8, 30, p_color, 4.5, 0.6)

    # 5. Wall bounce checks
    if new_bx <= 0 or new_bx >= 784:  # 800 - 16
        ball_dx = -ball_dx
        doodle.play_synth(196.0, 0.05, doodle.WAVE_SINE, 0.005, 0.02, 0.5, 0.02)
        doodle.shake_camera(2.0, 0.1)

    if new_by <= 0:
        ball_dy = -ball_dy
        doodle.play_synth(196.0, 0.05, doodle.WAVE_SINE, 0.005, 0.02, 0.5, 0.02)
        doodle.shake_camera(2.0, 0.1)

    # Ball falls off screen
    if new_by >= 600:
        state["lives"] -= 1

        # Procedural synth sound (life lost)
        doodle.play_synth(110.0, 0.35, doodle.WAVE_SAWTOOTH, 0.01, 0.15, 0.1, 0.1)
        doodle.shake_camera(12.0, 0.4)
        doodle.spawn_particles(new_bx, 580, 50, "#f43f5e", 6.0, 0.8)

        if state["lives"] <= 0:
            game_over = True
            game_over_screen.show()
        else:
            # Reset ball position
            ball.position = (390, 350)
            ball_dx = 5.0
            ball_dy = -5.0

# Register update tick
doodle.register_tick_callback(game_loop_tick)

def main():
    doodle.run(
        layout="layout.html",
        style="styles.css",
        width=800,
        height=600,
        title="Doodle Powerhouse Breakout",
        state=state
    )

if __name__ == "__main__":
    main()
