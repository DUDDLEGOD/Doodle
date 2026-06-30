import doodle

# ── Game state ──
state = {"p1_score": 0, "p2_score": 0}

COURT_W = 800
COURT_H = 490
PADDLE_H = 100
PADDLE_W = 14
PADDLE_SPEED = 7.0
BALL_SPEED = 6.0
WIN_SCORE = 7

ball_dx = 0.0
ball_dy = 0.0
serving = True
game_over = False

# ── Node references ──
p1 = doodle.getNode("p1-paddle")
p2 = doodle.getNode("p2-paddle")
ball = doodle.getNode("ball")
serve_msg = doodle.getNode("serve-msg")
win_screen = doodle.getNode("win-screen")

def reset_ball():
    global ball_dx, ball_dy, serving
    ball.position = (390, 240)
    ball_dx = 0.0
    ball_dy = 0.0
    serving = True
    serve_msg.show()

def serve(direction):
    global ball_dx, ball_dy, serving
    ball_dx = BALL_SPEED * direction
    ball_dy = BALL_SPEED * 0.5
    serving = False
    serve_msg.hide()
    doodle.playSynth(523.25, 0.1, doodle.WAVE_TRIANGLE, 0.01, 0.02, 0.3, 0.02)

def rematch():
    global game_over
    state["p1_score"] = 0
    state["p2_score"] = 0
    game_over = False
    win_screen.hide()
    p1.position = (30, 195)
    p2.position = (756, 195)
    reset_ball()
    doodle.playSynth(440.0, 0.15, doodle.WAVE_TRIANGLE, 0.01, 0.05, 0.3, 0.05)

def score_point(player):
    global game_over
    if player == 1:
        state["p1_score"] += 1
        doodle.spawnParticles(760, 245, 30, "#38bdf8", 5.0, 0.6)
    else:
        state["p2_score"] += 1
        doodle.spawnParticles(40, 245, 30, "#f43f5e", 5.0, 0.6)

    doodle.playSynth(130.0, 0.3, doodle.WAVE_SAWTOOTH, 0.01, 0.1, 0.1, 0.1)
    doodle.shakeCamera(8.0, 0.3)

    if state["p1_score"] >= WIN_SCORE:
        game_over = True
        doodle.updateText("win-text", "PLAYER 1 WINS!")
        win_screen.show()
        doodle.playSynth(659.25, 0.3, doodle.WAVE_TRIANGLE, 0.01, 0.1, 0.5, 0.1)
    elif state["p2_score"] >= WIN_SCORE:
        game_over = True
        doodle.updateText("win-text", "PLAYER 2 WINS!")
        win_screen.show()
        doodle.playSynth(659.25, 0.3, doodle.WAVE_TRIANGLE, 0.01, 0.1, 0.5, 0.1)
    else:
        reset_ball()

def tick():
    global ball_dx, ball_dy

    if game_over:
        return

    # ── Serve ──
    if serving:
        if doodle.isKeyPressed(32):  # SPACE
            serve(1)
        return

    # ── Player 1 input (W/S) ──
    p1x, p1y = p1.position
    if doodle.isKeyDown(87):  # W
        p1y -= PADDLE_SPEED
    if doodle.isKeyDown(83):  # S
        p1y += PADDLE_SPEED
    p1y = max(0, min(COURT_H - PADDLE_H, p1y))

    # ── Player 2 input (Up/Down arrows) ──
    p2x, p2y = p2.position
    if doodle.isKeyDown(265):  # UP
        p2y -= PADDLE_SPEED
    if doodle.isKeyDown(264):  # DOWN
        p2y += PADDLE_SPEED
    p2y = max(0, min(COURT_H - PADDLE_H, p2y))

    # ── Move ball ──
    bx, by = ball.position
    new_bx = bx + ball_dx
    new_by = by + ball_dy

    # ── Batch update positions ──
    doodle.batchProcess(
        positions={
            "p1-paddle": (p1x, p1y),
            "p2-paddle": (p2x, p2y),
            "ball": (new_bx, new_by),
        }
    )

    # ── Top/bottom wall bounce ──
    if new_by <= 0:
        ball_dy = abs(ball_dy)
        doodle.playSynth(196.0, 0.04, doodle.WAVE_SINE, 0.005, 0.01, 0.3, 0.01)
    elif new_by >= COURT_H - 20:
        ball_dy = -abs(ball_dy)
        doodle.playSynth(196.0, 0.04, doodle.WAVE_SINE, 0.005, 0.01, 0.3, 0.01)

    # ── Paddle collisions ──
    if doodle.checkCollision("ball", "p1-paddle"):
        ball_dx = abs(ball_dx)
        # Angle based on hit position
        hit_offset = (new_by + 10 - (p1y + PADDLE_H / 2)) / (PADDLE_H / 2)
        ball_dy = hit_offset * BALL_SPEED
        doodle.playSynth(330.0, 0.06, doodle.WAVE_SQUARE, 0.005, 0.02, 0.4, 0.02)
        doodle.shakeCamera(3.0, 0.1)
        doodle.spawnParticles(new_bx, new_by + hit_offset*50, 10, "#38bdf8", 2.5, 0.3)

    if doodle.checkCollision("ball", "p2-paddle"):
        ball_dx = -abs(ball_dx)
        hit_offset = (new_by + 10 - (p2y + PADDLE_H / 2)) / (PADDLE_H / 2)
        ball_dy = hit_offset * BALL_SPEED
        doodle.playSynth(330.0, 0.06, doodle.WAVE_SQUARE, 0.005, 0.02, 0.4, 0.02)
        doodle.shakeCamera(3.0, 0.1)
        doodle.spawnParticles(new_bx, new_by + 10, 10, "#f43f5e", 2.5, 0.3)

    # ── Scoring ──
    if new_bx < -20:
        score_point(2)
    elif new_bx > COURT_W:
        score_point(1)

# ── Setup ──
doodle.registerTickCallback(tick)
doodle.setEventContext({"rematch": rematch})

doodle.run(
    layout="layout.html",
    style="styles.css",
    width=800,
    height=600,
    title="Doodle Pong",
    state=state,
)
