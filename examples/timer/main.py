import doodle
import time
import sys
import platform
import os

# State variables
start_time = time.perf_counter()

state = {
    "seconds": "0"
}

timer_node = None

def get_system_font():
    sys_name = platform.system()
    if sys_name == "Linux":
        paths = [
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
            "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        ]
        for p in paths:
            if os.path.exists(p):
                return p
    elif sys_name == "Windows":
        paths = [
            os.path.join(os.environ.get("SystemRoot", "C:\\Windows"), "Fonts", "segoeui.ttf"),
            os.path.join(os.environ.get("SystemRoot", "C:\\Windows"), "Fonts", "arial.ttf"),
        ]
        for p in paths:
            if os.path.exists(p):
                return p
    elif sys_name == "Darwin":
        paths = [
            "/System/Library/Fonts/Helvetica.ttc",
            "/System/Library/Fonts/Supplemental/Arial.ttf",
        ]
        for p in paths:
            if os.path.exists(p):
                return p
    return ""

def tick():
    global start_time, timer_node
    
    if timer_node is None:
        timer_node = doodle.getNode("timer")
        font_path = get_system_font()
        if font_path:
            timer_node.style.font_family = font_path
        
        # Apply CSS properties dynamically from Python to bypass C stylesheet parsing bugs
        app_node = doodle.getNode("app")
        app_node.style.background_color = "#000000"
        app_node.style.flex_direction = "column"
        app_node.style.width = "100%"
        app_node.style.height = "100%"
        app_node.style.align_items = "center"
        app_node.style.justify_content = "center"

        timer_node.style.color = "#ffffff"
        timer_node.style.font_size = "200"
        timer_node.style.text_align = "center"
        timer_node.style.width = "100%"
        timer_node.style.height = "220px"
    
    # Calculate elapsed time
    elapsed = time.perf_counter() - start_time
    total_seconds = int(elapsed)
    
    # Update state
    state["seconds"] = str(total_seconds)

    # Print diagnostics every second
    if int(elapsed * 10) % 10 == 0:
        app_node = doodle.getNode("app")
        print(f"[Timer Debug] Pos: {timer_node.position}, Size: {timer_node.width}x{timer_node.height}, Text: {timer_node.text!r}, Visible: {timer_node.visible}")
        print(f"[Style Debug] #app bg: {app_node.style.background_color!r}, #timer font-size: {timer_node.style.font_size!r}, color: {timer_node.style.color!r}")

    # Press Esc to exit
    if doodle.isKeyPressed(256):
        sys.exit(0)


# Register callbacks
doodle.registerTickCallback(tick)

def main():
    doodle.run(
        layout="layout.html",
        style="styles.css",
        width=400,
        height=400,
        title="Timer",
        state=state
    )

if __name__ == "__main__":
    main()
