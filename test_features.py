import doodle
import math

circle_direction = 1
circle_speed = 3.0

def tick():
    global circle_direction
    ship1 = doodle.get_node("ship1")
    ship2 = doodle.get_node("ship2")
    circle = doodle.get_node("test-circle")
    rect = doodle.get_node("test-rect")
    
    # 1. Animate image rotations
    # Get current rotation or default to 0.0
    r1 = ship1.style.rotation
    r1_val = float(r1) if r1 else 0.0
    ship1.style.rotation = (r1_val + 2.0) % 360.0
    
    r2 = ship2.style.rotation
    r2_val = float(r2) if r2 else 0.0
    ship2.style.rotation = (r2_val - 1.0) % 360.0
    
    # 2. Move circle horizontally
    cx, cy = circle.position
    cx += circle_direction * circle_speed
    if cx > 280.0:
        circle_direction = -1
    elif cx < 80.0:
        circle_direction = 1
    circle.position = (cx, cy)
    
    # 3. Verify collision detection
    if doodle.check_collision("test-circle", "test-rect"):
        rect.style.background_color = "#f59e0b" # Orange match
    else:
        rect.style.background_color = "#8b5cf6" # Default purple

doodle.register_tick_callback(tick)

def main():
    doodle.run(
        layout="test_features_layout.html",
        style="examples/breakout/styles.css",
        width=800,
        height=600,
        title="Doodle Features Verification",
        state={}
    )

if __name__ == "__main__":
    main()
