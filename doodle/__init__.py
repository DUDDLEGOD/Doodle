import _doodle
import sys
import re
import time
import math

# Re-export all flat APIs from _doodle
for name in dir(_doodle):
    if not name.startswith('_'):
        globals()[name] = getattr(_doodle, name)

# Event listener registry
# node_id -> event_type -> list of callables
_event_listeners = {}

# Attribute listeners context
_event_context = {}

def add_event_listener(node_id, event_type, callback):
    if node_id not in _event_listeners:
        _event_listeners[node_id] = {}
    if event_type not in _event_listeners[node_id]:
        _event_listeners[node_id][event_type] = []
    _event_listeners[node_id][event_type].append(callback)

def set_event_context(context_dict):
    global _event_context
    _event_context.update(context_dict)

# OOP Wrapper Node Class
class NodeStyleProxy:
    def __init__(self, node_id):
        self._id = node_id
    def __setattr__(self, name, value):
        if name.startswith('_'):
            super().__setattr__(name, value)
        else:
            # Convert python_name to css-property-name (e.g. background_color -> background-color)
            css_name = name.replace('_', '-')
            _doodle.set_style(self._id, css_name, str(value))
    def __getattr__(self, name):
        return ""

class Node:
    def __init__(self, node_id):
        self.id = node_id
        self.style = NodeStyleProxy(node_id)

    @property
    def position(self):
        return _doodle.get_position(self.id)

    @position.setter
    def position(self, pos):
        _doodle.set_position(self.id, float(pos[0]), float(pos[1]))

    @property
    def x(self):
        return _doodle.get_position(self.id)[0]

    @x.setter
    def x(self, val):
        y = _doodle.get_position(self.id)[1]
        _doodle.set_position(self.id, float(val), y)

    @property
    def y(self):
        return _doodle.get_position(self.id)[1]

    @y.setter
    def y(self, val):
        x = _doodle.get_position(self.id)[0]
        _doodle.set_position(self.id, x, float(val))

    @property
    def text(self):
        return ""

    @text.setter
    def text(self, value):
        _doodle.update_text(self.id, str(value))

    def show(self):
        _doodle.show_node(self.id)

    def hide(self):
        _doodle.hide_node(self.id)

def get_node(node_id):
    return Node(node_id)

# Tweens Animation Engine
_active_tweens = []

def animate(node_id, target_x=None, target_y=None, duration=0.5, ease="quad_out"):
    node = Node(node_id)
    start_x, start_y = node.position
    
    if target_x is not None:
        _active_tweens.append({
            "node": node,
            "prop": "x",
            "start": start_x,
            "end": target_x,
            "duration": duration,
            "elapsed": 0.0,
            "ease": ease
        })
    if target_y is not None:
        _active_tweens.append({
            "node": node,
            "prop": "y",
            "start": start_y,
            "end": target_y,
            "duration": duration,
            "elapsed": 0.0,
            "ease": ease
        })

def _update_tweens(dt):
    global _active_tweens
    still_active = []
    for t in _active_tweens:
        t["elapsed"] += dt
        progress = min(t["elapsed"] / t["duration"], 1.0)
        
        if t["ease"] == "linear":
            val = progress
        elif t["ease"] == "quad_out":
            val = progress * (2 - progress)
        elif t["ease"] == "quad_in":
            val = progress * progress
        else:
            val = progress
            
        current_val = t["start"] + val * (t["end"] - t["start"])
        setattr(t["node"], t["prop"], current_val)
        
        if t["elapsed"] < t["duration"]:
            still_active.append(t)
    _active_tweens = still_active

# Waveform Constants
WAVE_SINE = 0
WAVE_SQUARE = 1
WAVE_TRIANGLE = 2
WAVE_SAWTOOTH = 3
WAVE_NOISE = 4

# Template Data Binding
_templates = {}

def _parse_layout_templates(layout_path):
    global _templates
    _templates = {}
    try:
        with open(layout_path, "r", encoding="utf-8") as f:
            content = f.read()
            
        # Match elements containing {{ variable }} with template group markers
        pattern = r'<([a-zA-Z0-9]+)\s+[^>]*id="([^"]+)"[^>]*>([^<]*\{\{.*?\}\}[^<]*)<\/\1>'
        matches = re.findall(pattern, content, re.DOTALL)
        for tag, node_id, template_str in matches:
            vars_found = re.findall(r'\{\{\s*([a-zA-Z0-9_]+)\s*\}\}', template_str)
            _templates[node_id] = (template_str, vars_found)
    except Exception as e:
        print(f"Template parsing warning: {e}")

def _update_templates(state):
    for node_id, (template_str, vars_found) in _templates.items():
        rendered = template_str
        for var in vars_found:
            val = state.get(var, "")
            rendered = re.sub(r'\{\{\s*' + var + r'\s*\}\}', str(val), rendered)
        _doodle.update_text(node_id, rendered)

# Extended main run loop
def run(layout="layout.html", style="styles.css", width=800, height=600, title="Doodle Engine", state=None):
    _parse_layout_templates(layout)
    
    # Extract inline event callbacks from layout.html
    _inline_listeners = {}
    try:
        with open(layout, "r", encoding="utf-8") as f:
            content = f.read()
        tag_pattern = r'<[a-zA-Z0-9]+\s+([^>]*id="([^"]+)"[^>]*)>'
        tags = re.findall(tag_pattern, content)
        for attr_str, node_id in tags:
            click_match = re.search(r'onclick="([^"]+)"', attr_str)
            if click_match:
                cb_name = click_match.group(1)
                _inline_listeners[(node_id, "click")] = cb_name
            hover_match = re.search(r'onhover="([^"]+)"', attr_str)
            if hover_match:
                cb_name = hover_match.group(1)
                _inline_listeners[(node_id, "hover")] = cb_name
    except Exception as e:
        print(f"Event parsing warning: {e}")

    last_time = time.perf_counter()
    original_callback = None
    
    def wrapper_tick():
        nonlocal last_time
        now = time.perf_counter()
        dt = now - last_time
        last_time = now
        
        _update_tweens(dt)
        
        if state is not None:
            _update_templates(state)
            
        all_ids = set(list(_event_listeners.keys()) + [k[0] for k in _inline_listeners.keys()])
        for node_id in all_ids:
            if _doodle.is_node_clicked(node_id):
                if node_id in _event_listeners and "click" in _event_listeners[node_id]:
                    for cb in _event_listeners[node_id]["click"]:
                        cb()
                if (node_id, "click") in _inline_listeners:
                    cb_name = _inline_listeners[(node_id, "click")]
                    cb = _event_context.get(cb_name) or globals().get(cb_name) or sys.modules['__main__'].__dict__.get(cb_name)
                    if cb:
                        cb()
                        
            if _doodle.is_node_hovered(node_id):
                if node_id in _event_listeners and "hover" in _event_listeners[node_id]:
                    for cb in _event_listeners[node_id]["hover"]:
                        cb()
                if (node_id, "hover") in _inline_listeners:
                    cb_name = _inline_listeners[(node_id, "hover")]
                    cb = _event_context.get(cb_name) or globals().get(cb_name) or sys.modules['__main__'].__dict__.get(cb_name)
                    if cb:
                        cb()
                        
        if original_callback:
            original_callback()

    def register_tick_callback(callback):
        nonlocal original_callback
        original_callback = callback

    # Setup Python wrapper's registration hook
    _doodle.register_tick_callback(wrapper_tick)
    globals()["register_tick_callback"] = register_tick_callback
    
    return _doodle.run(layout=layout, style=style, width=width, height=height, title=title)
