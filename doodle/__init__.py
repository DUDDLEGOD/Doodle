"""
Doodle UI Engine Python Wrapper
Hardware-accelerated DOM-based UI & 2D Game Engine
"""

from typing import Callable, Any, Optional, Dict, List, Tuple, Union
import sys
import os
import re
import time

try:
    from . import _doodle
except ImportError:
    import _doodle

# Re-export all flat APIs from _doodle
for name in dir(_doodle):
    if not name.startswith('_'):
        globals()[name] = getattr(_doodle, name)

# Event listener registry
# node_id -> event_type -> list of callables
_event_listeners: Dict[str, Dict[str, List[Callable[[], None]]]] = {}
_event_context: Dict[str, Any] = {}
_inline_listeners: Dict[Tuple[str, str], str] = {}
_all_event_nodes: List[str] = []

def _rebuild_event_nodes() -> None:
    global _all_event_nodes
    _all_event_nodes = list(set(list(_event_listeners.keys()) + [k[0] for k in _inline_listeners.keys()]))

def addEventListener(node_id: str, event_type: str, callback: Callable[[], None]) -> None:
    if node_id not in _event_listeners:
        _event_listeners[node_id] = {}
    if event_type not in _event_listeners[node_id]:
        _event_listeners[node_id][event_type] = []
    _event_listeners[node_id][event_type].append(callback)
    _rebuild_event_nodes()

add_event_listener = addEventListener  # Backward compatibility alias

def setEventContext(context_dict: Dict[str, Any]) -> None:
    global _event_context
    _event_context.update(context_dict)

set_event_context = setEventContext  # Backward compatibility alias

def _raise_property_error(node_id: str, prop_name: str, expected_type: str, actual_value: Any) -> None:
    try:
        line_num = _doodle.getLineNumber(node_id)
        location = f"layout.html:L{line_num}"
    except Exception:
        location = "unknown location"
    
    msg = (
        f"Invalid value for property '{prop_name}' on node '{node_id}' (defined at {location}).\n"
        f"Expected: {expected_type}\n"
        f"Got: {repr(actual_value)} (type: {type(actual_value).__name__})"
    )
    raise TypeError(msg)

# OOP Wrapper Node Class
class NodeStyleProxy:
    def __init__(self, node_id: str):
        self._id = node_id

    def __setattr__(self, name: str, value: Any) -> None:
        if name.startswith('_'):
            super().__setattr__(name, value)
        else:
            # Convert python_name to css-property-name (e.g. background_color -> background-color)
            css_name = name.replace('_', '-')
            _doodle.setStyle(self._id, css_name, str(value))

    def __getattr__(self, name: str) -> Union[str, float, int]:
        if name.startswith('_'):
            raise AttributeError(name)
        css_name = name.replace('_', '-')
        val = _doodle.getStyle(self._id, css_name)
        if not val:
            return ""
        try:
            if '.' in val:
                return float(val)
            return int(val)
        except ValueError:
            return val

class Node:
    def __init__(self, node_id: str):
        self.id: str = node_id
        self.style: NodeStyleProxy = NodeStyleProxy(node_id)

    @property
    def position(self) -> Tuple[float, float]:
        return _doodle.getPosition(self.id)

    @position.setter
    def position(self, pos: Union[Tuple[float, float], List[float]]) -> None:
        if not isinstance(pos, (tuple, list)) or len(pos) < 2:
            _raise_property_error(self.id, "position", "tuple or list of 2 numbers", pos)
        try:
            val_x = float(pos[0])
            val_y = float(pos[1])
        except (ValueError, TypeError):
            _raise_property_error(self.id, "position", "tuple or list of 2 numbers", pos)
        _doodle.setPosition(self.id, val_x, val_y)

    @property
    def x(self) -> float:
        return _doodle.getPosition(self.id)[0]

    @x.setter
    def x(self, val: float) -> None:
        try:
            val_f = float(val)
        except (ValueError, TypeError):
            _raise_property_error(self.id, "x", "float or int", val)
        y = _doodle.getPosition(self.id)[1]
        _doodle.setPosition(self.id, val_f, y)

    @property
    def y(self) -> float:
        return _doodle.getPosition(self.id)[1]

    @y.setter
    def y(self, val: float) -> None:
        try:
            val_f = float(val)
        except (ValueError, TypeError):
            _raise_property_error(self.id, "y", "float or int", val)
        x = _doodle.getPosition(self.id)[0]
        _doodle.setPosition(self.id, x, val_f)

    @property
    def width(self) -> float:
        return _doodle.getLayoutSize(self.id)[0]

    @width.setter
    def width(self, val: float) -> None:
        try:
            val_f = float(val)
        except (ValueError, TypeError):
            _raise_property_error(self.id, "width", "float or int", val)
        _doodle.setStyle(self.id, "width", f"{val_f}px")

    @property
    def w(self) -> float:
        return self.width

    @w.setter
    def w(self, val: float) -> None:
        self.width = val

    @property
    def height(self) -> float:
        return _doodle.getLayoutSize(self.id)[1]

    @height.setter
    def height(self, val: float) -> None:
        try:
            val_f = float(val)
        except (ValueError, TypeError):
            _raise_property_error(self.id, "height", "float or int", val)
        _doodle.setStyle(self.id, "height", f"{val_f}px")

    @property
    def h(self) -> float:
        return self.height

    @h.setter
    def h(self, val: float) -> None:
        self.height = val

    @property
    def text(self) -> str:
        return _doodle.getText(self.id)

    @text.setter
    def text(self, value: Any) -> None:
        _doodle.updateText(self.id, str(value))

    @property
    def visible(self) -> bool:
        return bool(_doodle.isVisible(self.id))

    @visible.setter
    def visible(self, val: Any) -> None:
        is_vis = bool(val)
        if is_vis:
            _doodle.showNode(self.id)
        else:
            _doodle.hideNode(self.id)

    def show(self) -> None:
        _doodle.showNode(self.id)

    def hide(self) -> None:
        _doodle.hideNode(self.id)

def getNode(node_id: str) -> Node:
    return Node(node_id)

get_node = getNode  # Backward compatibility alias

# Tweens Animation Engine
_active_tweens: List[Dict[str, Any]] = []

def animate(node_id: str, target_x: Optional[float] = None, target_y: Optional[float] = None, duration: float = 0.5, ease: str = "quad_out") -> None:
    node = getNode(node_id)
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

def _update_tweens(dt: float) -> None:
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
WAVE_SINE: int = 0
WAVE_SQUARE: int = 1
WAVE_TRIANGLE: int = 2
WAVE_SAWTOOTH: int = 3
WAVE_NOISE: int = 4

# Template Data Binding
_templates: Dict[str, str] = {}

def _parse_layout_templates(layout_path: str) -> None:
    global _templates
    _templates = {}
    try:
        with open(layout_path, "r", encoding="utf-8") as f:
            content = f.read()
            
        # Match elements containing {{ variable }} with template group markers
        pattern = r'<([a-zA-Z0-9]+)\s+[^>]*id="([^"]+)"[^>]*>([^<]*\{\{.*?\}\}[^<]*)<\/\1>'
        matches = re.findall(pattern, content, re.DOTALL)
        for tag, node_id, template_str in matches:
            # Convert '{{ var }}' to '{var}' for built-in fast formatting
            _templates[node_id] = re.sub(r'\{\{\s*([a-zA-Z0-9_]+)\s*\}\}', r'{\1}', template_str)
    except Exception as e:
        print(f"Template parsing warning: {e}")

_tick_callback: Optional[Callable[[], None]] = None

def registerTickCallback(callback: Callable[[], None]) -> None:
    global _tick_callback
    _tick_callback = callback

register_tick_callback = registerTickCallback  # Backward compatibility alias

_last_rendered_templates: Dict[str, str] = {}

def _update_templates(state: Dict[str, Any]) -> None:
    for node_id, format_str in _templates.items():
        try:
            formatted = format_str.format(**state)
            if _last_rendered_templates.get(node_id) != formatted:
                _doodle.updateText(node_id, formatted)
                _last_rendered_templates[node_id] = formatted
        except Exception:
            pass

def _default_console_callback(cmd: str) -> str:
    import io
    main_mod = sys.modules.get('__main__')
    namespace = main_mod.__dict__ if main_mod else globals()
    
    old_stdout = sys.stdout
    new_stdout = io.StringIO()
    sys.stdout = new_stdout
    
    try:
        try:
            code = compile(cmd.strip(), '<console>', 'eval')
            res = eval(code, namespace, namespace)
            if res is not None:
                print(res)
        except Exception:
            code = compile(cmd, '<console>', 'exec')
            exec(code, namespace, namespace)
    except Exception as e:
        print(f"Error: {e}")
    finally:
        sys.stdout = old_stdout
        
    return new_stdout.getvalue().rstrip('\n')

# Extended main run loop
def run(layout: str = "layout.html", style: str = "styles.css", width: int = 800, height: int = 600, title: str = "Doodle Engine", state: Optional[Dict[str, Any]] = None) -> Any:
    # Automatically change working directory to the directory of the script being run
    if sys.argv and sys.argv[0]:
        script_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
        if script_dir and os.path.isdir(script_dir):
            try:
                os.chdir(script_dir)
            except Exception as e:
                print(f"Directory change warning: {e}")

    if hasattr(sys, "_MEIPASS"):
        try:
            os.chdir(sys._MEIPASS)
        except Exception:
            pass

    # Print start info & validate file existence
    layout_abs = os.path.abspath(layout)
    style_abs = os.path.abspath(style)
    print(f"[Doodle Engine] Working Directory: {os.getcwd()}")
    print(f"[Doodle Engine] Layout Path: {layout_abs} (Exists: {os.path.exists(layout_abs)})")
    print(f"[Doodle Engine] Style Path: {style_abs} (Exists: {os.path.exists(style_abs)})")

    if not os.path.exists(layout_abs):
        raise FileNotFoundError(f"Doodle Layout file not found: {layout_abs}")
    if not os.path.exists(style_abs):
        raise FileNotFoundError(f"Doodle Style file not found: {style_abs}")

    _parse_layout_templates(layout)
    
    # Extract inline event callbacks from layout.html
    global _inline_listeners
    _inline_listeners = {}
    try:
        with open(layout, "r", encoding="utf-8") as f:
            content = f.read()
        tag_pattern = r'<[a-zA-Z0-9]+\s+([^>]*id="([^"]+)"[^>]*)>'
        tags = re.findall(tag_pattern, content)
        for attr_str, node_id in tags:
            click_match = re.search(r'onclick="([^"]+)"', attr_str)
            if click_match:
                _inline_listeners[(node_id, "click")] = click_match.group(1)
            hover_match = re.search(r'onhover="([^"]+)"', attr_str)
            if hover_match:
                _inline_listeners[(node_id, "hover")] = hover_match.group(1)
    except Exception as e:
        print(f"Event parsing warning: {e}")
        
    _rebuild_event_nodes()

    last_time = time.perf_counter()
    frame_count = 0
    
    def wrapper_tick() -> None:
        nonlocal last_time, frame_count
        now = time.perf_counter()
        dt = now - last_time
        last_time = now
        
        frame_count += 1
        if frame_count % 300 == 0:
            import gc
            gc.collect()
        
        _update_tweens(dt)
        
        if state is not None:
            _update_templates(state)
            
        for node_id in _all_event_nodes:
            if _doodle.isNodeClicked(node_id):
                if node_id in _event_listeners and "click" in _event_listeners[node_id]:
                    for cb in _event_listeners[node_id]["click"]:
                        cb()
                if (node_id, "click") in _inline_listeners:
                    cb_name = _inline_listeners[(node_id, "click")]
                    cb = _event_context.get(cb_name) or globals().get(cb_name) or sys.modules['__main__'].__dict__.get(cb_name)
                    if cb:
                        cb()
                        
            if _doodle.isNodeHovered(node_id):
                if node_id in _event_listeners and "hover" in _event_listeners[node_id]:
                    for cb in _event_listeners[node_id]["hover"]:
                        cb()
                if (node_id, "hover") in _inline_listeners:
                    cb_name = _inline_listeners[(node_id, "hover")]
                    cb = _event_context.get(cb_name) or globals().get(cb_name) or sys.modules['__main__'].__dict__.get(cb_name)
                    if cb:
                        cb()
                        
        if _tick_callback:
            _tick_callback()

    # Setup Python wrapper's registration hook
    _doodle.registerTickCallback(wrapper_tick)
    _doodle.registerConsoleCallback(_default_console_callback)
    
    return _doodle.run(layout=layout, style=style, width=width, height=height, title=title)
