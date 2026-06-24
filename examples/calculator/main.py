import doodle
import math

# State
current_input = "0"
expression_string = ""
current_theme_idx = 0

# Themes
THEMES = [
    {   # Dark (Default)
        "bg": "#242424",
        "disp_bg": "#1e1e1e",
        "num_bg": "#333333",
        "num_fg": "#ffffff",
        "op_bg": "#a5a5a5",
        "op_fg": "#000000",
        "acc_bg": "#ff9f0a",
        "acc_fg": "#ffffff",
        "txt": "#ffffff",
        "exp": "#888888"
    },
    {   # Light
        "bg": "#f0f0f0",
        "disp_bg": "#ffffff",
        "num_bg": "#e0e0e0",
        "num_fg": "#000000",
        "op_bg": "#d0d0d0",
        "op_fg": "#000000",
        "acc_bg": "#007aff",
        "acc_fg": "#ffffff",
        "txt": "#000000",
        "exp": "#666666"
    },
    {   # Neon Retro
        "bg": "#09090b",
        "disp_bg": "#18181b",
        "num_bg": "#27272a",
        "num_fg": "#22d3ee",
        "op_bg": "#3f3f46",
        "op_fg": "#c084fc",
        "acc_bg": "#e879f9",
        "acc_fg": "#000000",
        "txt": "#22d3ee",
        "exp": "#a1a1aa"
    }
]

# Nodes
display_exp = doodle.getNode("expression")
display_res = doodle.getNode("result")
calculator_bg = doodle.getNode("calculator")
display_area = doodle.getNode("display-area")

# Node Groups
num_btns = ["btn-0", "btn-1", "btn-2", "btn-3", "btn-4", "btn-5", "btn-6", "btn-7", "btn-8", "btn-9", "btn-dot"]
op_btns = ["btn-c", "btn-paren-l", "btn-paren-r", "btn-sign", "btn-theme"]
acc_btns = ["btn-div", "btn-mul", "btn-sub", "btn-add", "btn-eq"]

all_btns = num_btns + op_btns + acc_btns

# Animation tracking
# Store original background colors to revert to after animation
btn_base_colors = {}
anim_timers = {}

def apply_theme(idx):
    t = THEMES[idx]
    calculator_bg.style.background_color = t["bg"]
    display_area.style.background_color = t["disp_bg"]
    display_res.style.color = t["txt"]
    display_exp.style.color = t["exp"]

    for b in num_btns:
        node = doodle.getNode(b)
        node.style.background_color = t["num_bg"]
        node.style.color = t["num_fg"]
        btn_base_colors[b] = t["num_bg"]

    for b in op_btns:
        node = doodle.getNode(b)
        node.style.background_color = t["op_bg"]
        node.style.color = t["op_fg"]
        btn_base_colors[b] = t["op_bg"]

    for b in acc_btns:
        node = doodle.getNode(b)
        node.style.background_color = t["acc_bg"]
        node.style.color = t["acc_fg"]
        btn_base_colors[b] = t["acc_bg"]

def trigger_anim(btn_id):
    # Flash button white
    node = doodle.getNode(btn_id)
    node.style.background_color = "#ffffff"
    # Keep track of animation timer (in frames, roughly 6 frames = 0.1s)
    anim_timers[btn_id] = 6
    # Animate slight position bump
    start_y = node.y
    doodle.animate(btn_id, target_y=start_y + 2, duration=0.05, ease="linear")

def play_click():
    doodle.playSynth(880.0, 0.05, doodle.WAVE_SINE, 0.01, 0.02, 0.5, 0.02)

def update_display():
    global current_input, expression_string
    display_exp.text = expression_string
    display_res.text = current_input

def handle_num(n: str, btn_id: str):
    global current_input
    if current_input == "0" or current_input == "Error":
        current_input = n
    else:
        if len(current_input) < 12:
            current_input += n
    play_click()
    trigger_anim(btn_id)
    update_display()

def handle_op(op: str, btn_id: str):
    global current_input, expression_string
    if current_input != "Error":
        expression_string += current_input + " " + op + " "
        current_input = "0"
    play_click()
    trigger_anim(btn_id)
    update_display()

def calc_result(btn_id: str):
    global current_input, expression_string
    try:
        if expression_string or current_input != "0":
            full_expr = expression_string + current_input
            # Basic eval for calculator demo (replace X with *)
            eval_expr = full_expr.replace("x", "*")
            res = eval(eval_expr)
            current_input = str(round(res, 8))
            expression_string = ""
    except Exception:
        current_input = "Error"
        expression_string = ""
    play_click()
    trigger_anim(btn_id)
    update_display()

def clear_all(btn_id: str):
    global current_input, expression_string
    current_input = "0"
    expression_string = ""
    play_click()
    trigger_anim(btn_id)
    update_display()

def handle_paren(p: str, btn_id: str):
    global current_input, expression_string
    if current_input == "0":
        expression_string += p + " "
    else:
        expression_string += current_input + " " + p + " "
        current_input = "0"
    play_click()
    trigger_anim(btn_id)
    update_display()

def toggle_sign(btn_id: str):
    global current_input
    if current_input != "0" and current_input != "Error":
        if current_input.startswith("-"):
            current_input = current_input[1:]
        else:
            current_input = "-" + current_input
    play_click()
    trigger_anim(btn_id)
    update_display()

def add_dot(btn_id: str):
    global current_input
    if current_input != "Error" and "." not in current_input:
        current_input += "."
    play_click()
    trigger_anim(btn_id)
    update_display()

def cycle_theme(btn_id: str):
    global current_theme_idx
    current_theme_idx = (current_theme_idx + 1) % len(THEMES)
    apply_theme(current_theme_idx)
    play_click()
    trigger_anim(btn_id)

# Setup Event Listeners (Mouse Clicks)
handlers = {
    "btn-0": lambda: handle_num("0", "btn-0"),
    "btn-1": lambda: handle_num("1", "btn-1"),
    "btn-2": lambda: handle_num("2", "btn-2"),
    "btn-3": lambda: handle_num("3", "btn-3"),
    "btn-4": lambda: handle_num("4", "btn-4"),
    "btn-5": lambda: handle_num("5", "btn-5"),
    "btn-6": lambda: handle_num("6", "btn-6"),
    "btn-7": lambda: handle_num("7", "btn-7"),
    "btn-8": lambda: handle_num("8", "btn-8"),
    "btn-9": lambda: handle_num("9", "btn-9"),
    
    "btn-add": lambda: handle_op("+", "btn-add"),
    "btn-sub": lambda: handle_op("-", "btn-sub"),
    "btn-mul": lambda: handle_op("x", "btn-mul"),
    "btn-div": lambda: handle_op("/", "btn-div"),
    
    "btn-eq": lambda: calc_result("btn-eq"),
    "btn-c": lambda: clear_all("btn-c"),
    "btn-paren-l": lambda: handle_paren("(", "btn-paren-l"),
    "btn-paren-r": lambda: handle_paren(")", "btn-paren-r"),
    "btn-sign": lambda: toggle_sign("btn-sign"),
    "btn-dot": lambda: add_dot("btn-dot"),
    "btn-theme": lambda: cycle_theme("btn-theme")
}

for btn_id, handler in handlers.items():
    doodle.addEventListener(btn_id, "click", handler)

def tick():
    # Keyboard Handling (Numpad + Numbers + Operators)
    # Check numbers
    for i in range(10):
        # Raylib keycodes: top row numbers are 48-57, numpad are 320-329
        if doodle.isKeyPressed(48 + i) or doodle.isKeyPressed(320 + i):
            handle_num(str(i), f"btn-{i}")
            
    # Operators
    if doodle.isKeyPressed(331): handle_op("/", "btn-div") # Numpad /
    if doodle.isKeyPressed(332): handle_op("x", "btn-mul") # Numpad *
    if doodle.isKeyPressed(333) or doodle.isKeyPressed(45): handle_op("-", "btn-sub") # Numpad - or normal -
    if doodle.isKeyPressed(334): handle_op("+", "btn-add") # Numpad +
    
    # Dot/Decimal
    if doodle.isKeyPressed(330) or doodle.isKeyPressed(46): add_dot("btn-dot") # Numpad . or normal .
    
    # Enter / Equals
    if doodle.isKeyPressed(257) or doodle.isKeyPressed(335) or doodle.isKeyPressed(61): # Enter, Numpad Enter, '='
        calc_result("btn-eq")
        
    # Clear (Backspace or C or Esc)
    if doodle.isKeyPressed(259) or doodle.isKeyPressed(67) or doodle.isKeyPressed(256):
        clear_all("btn-c")

    # Animation ticking
    expired = []
    for btn_id, timer in anim_timers.items():
        if timer > 0:
            anim_timers[btn_id] -= 1
            if anim_timers[btn_id] <= 0:
                expired.append(btn_id)
                
    for btn_id in expired:
        node = doodle.getNode(btn_id)
        # Restore color
        node.style.background_color = btn_base_colors.get(btn_id, "#000000")
        # Restore y position slowly
        doodle.animate(btn_id, target_y=node.y - 2, duration=0.1, ease="quad_out")
        del anim_timers[btn_id]

doodle.registerTickCallback(tick)

def main():
    apply_theme(0) # Init default theme
    doodle.run(
        layout="layout.html",
        style="styles.css",
        width=400,
        height=600,
        title="Doodle Calculator"
    )

if __name__ == "__main__":
    main()
