"""
Type stubs for the Doodle UI Engine.
Provides complete type annotations and docstrings for IDE support.
"""

from typing import Callable, Any, Optional, Dict, List, Tuple, Union

# Constant Definitions
WAVE_SINE: int
WAVE_SQUARE: int
WAVE_TRIANGLE: int
WAVE_SAWTOOTH: int
WAVE_NOISE: int

class NodeStyleProxy:
    """
    Proxy to dynamically get and set CSS style properties on a DOM node.
    Property names containing underscores are automatically mapped to hyphenated CSS properties
    (e.g., `node.style.background_color = "red"` maps to `background-color: red`).
    """
    def __init__(self, node_id: str) -> None: ...
    def __setattr__(self, name: str, value: Any) -> None: ...
    def __getattr__(self, name: str) -> Union[str, float, int]: ...

class Node:
    """
    Object-oriented wrapper representing a single DOM node in the Doodle UI layout tree.
    Allows intuitive, high-performance property accessors and lifecycle operations.
    """
    id: str
    style: NodeStyleProxy

    def __init__(self, node_id: str) -> None: ...

    @property
    def position(self) -> Tuple[float, float]:
        """Get or set the (x, y) absolute layout position of the node."""
        ...

    @position.setter
    def position(self, pos: Union[Tuple[float, float], List[float]]) -> None: ...

    @property
    def x(self) -> float:
        """Get or set the X coordinate of the node's layout box."""
        ...

    @x.setter
    def x(self, val: float) -> None: ...

    @property
    def y(self) -> float:
        """Get or set the Y coordinate of the node's layout box."""
        ...

    @y.setter
    def y(self, val: float) -> None: ...

    @property
    def width(self) -> float:
        """Get or set the calculated width of the node's layout box."""
        ...

    @width.setter
    def width(self, val: float) -> None: ...

    @property
    def w(self) -> float:
        """Alias for `width`."""
        ...

    @w.setter
    def w(self, val: float) -> None: ...

    @property
    def height(self) -> float:
        """Get or set the calculated height of the node's layout box."""
        ...

    @height.setter
    def height(self, val: float) -> None: ...

    @property
    def h(self) -> float:
        """Alias for `height`."""
        ...

    @h.setter
    def h(self, val: float) -> None: ...

    @property
    def text(self) -> str:
        """Get or set the inner text content of the node."""
        ...

    @text.setter
    def text(self, value: Any) -> None: ...

    @property
    def visible(self) -> bool:
        """Get or set the visibility of the node. Setting to False hides the node, True shows it."""
        ...

    @visible.setter
    def visible(self, val: Any) -> None: ...

    def show(self) -> None:
        """Make the node visible in the layout tree."""
        ...

    def hide(self) -> None:
        """Hide the node from rendering and disable its collision checks."""
        ...

def getNode(node_id: str) -> Node:
    """
    Retrieve an OOP wrapper for a layout node by its ID.
    Raises KeyError immediately if the node is not found in the layout tree.
    """
    ...

def get_node(node_id: str) -> Node:
    """Backward compatibility alias for `getNode`."""
    ...

def addEventListener(node_id: str, event_type: str, callback: Callable[[], None]) -> None:
    """Register an event listener (e.g., 'click', 'hover') for a specific node ID."""
    ...

def add_event_listener(node_id: str, event_type: str, callback: Callable[[], None]) -> None:
    """Backward compatibility alias for `addEventListener`."""
    ...

def setEventContext(context_dict: Dict[str, Any]) -> None:
    """Set the evaluation namespace context for resolving layout-defined inline string event handlers."""
    ...

def set_event_context(context_dict: Dict[str, Any]) -> None:
    """Backward compatibility alias for `setEventContext`."""
    ...

def registerTickCallback(callback: Callable[[], None]) -> None:
    """Register a global tick callback function to run once per frame in the main engine loop."""
    ...

def register_tick_callback(callback: Callable[[], None]) -> None:
    """Backward compatibility alias for `registerTickCallback`."""
    ...

def animate(node_id: str, target_x: Optional[float] = None, target_y: Optional[float] = None, duration: float = 0.5, ease: str = "quad_out") -> None:
    """Smoothly animate a node's x or y position over a specified duration with easing."""
    ...

def run(layout: str = "layout.html", style: str = "styles.css", width: int = 800, height: int = 600, title: str = "Doodle Engine", state: Optional[Dict[str, Any]] = None) -> Any:
    """
    Initialize Raylib and start the main hardware-accelerated rendering and event loop.
    Enables automatic hot-reloading for layout and style changes.
    """
    ...

# Native binding functions from _doodle re-exported flat:
def checkCollision(id_a: str, id_b: str) -> bool:
    """Check if the physical bounding shapes of two nodes are colliding (supports Circle-Circle, Circle-Rect, and Rect-Rect)."""
    ...

def check_collision(id_a: str, id_b: str) -> bool: ...

def getFirstCollision(id: str, group: str) -> Optional[str]:
    """Retrieve the ID of the first node in a specific group/class that is colliding with the target node."""
    ...

def get_first_collision(id: str, group: str) -> Optional[str]: ...

def setPosition(id: str, x: float, y: float) -> bool:
    """Directly set the layout coordinates of a node."""
    ...

def set_position(id: str, x: float, y: float) -> bool: ...

def getPosition(id: str) -> Tuple[float, float]:
    """Get the current layout coordinates of a node."""
    ...

def get_position(id: str) -> Tuple[float, float]: ...

def removeNode(id: str) -> bool:
    """Permanently remove a node from the DOM layout tree."""
    ...

def remove_node(id: str) -> bool: ...

def updateText(id: str, text: str) -> bool:
    """Update the text content of a node."""
    ...

def update_text(id: str, text: str) -> bool: ...

def showNode(id: str) -> bool:
    """Show a node."""
    ...

def show_node(id: str) -> bool: ...

def hideNode(id: str) -> bool:
    """Hide a node."""
    ...

def hide_node(id: str) -> bool: ...

def playSound(sound_path: str) -> None:
    """Play a cached sound file."""
    ...

def play_sound(sound_path: str) -> None: ...

def isKeyDown(key: int) -> bool:
    """Check if a keyboard key is currently being held down."""
    ...

def is_key_down(key: int) -> bool: ...

def isKeyPressed(key: int) -> bool:
    """Check if a keyboard key was pressed during the current frame."""
    ...

def is_key_pressed(key: int) -> bool: ...

def getMouseX() -> int:
    """Get the current mouse pointer X position."""
    ...

def get_mouse_x() -> int: ...

def getMouseY() -> int:
    """Get the current mouse pointer Y position."""
    ...

def get_mouse_y() -> int: ...

def getMousePosition() -> Tuple[float, float]:
    """Get the current mouse pointer (x, y) coordinates."""
    ...

def get_mouse_position() -> Tuple[float, float]: ...

def isMouseButtonDown(button: int) -> bool:
    """Check if a mouse button is currently being held down."""
    ...

def is_mouse_button_down(button: int) -> bool: ...

def isMouseButtonPressed(button: int) -> bool:
    """Check if a mouse button was clicked during the current frame."""
    ...

def is_mouse_button_pressed(button: int) -> bool: ...

def getMouseWheelMove() -> float:
    """Get the mouse wheel scrolling delta."""
    ...

def get_mouse_wheel_move() -> float: ...

def setMouseCursor(cursor_type: int) -> None:
    """Set the system mouse cursor visual style."""
    ...

def set_mouse_cursor(cursor_type: int) -> None: ...

def setWindowTitle(title: str) -> None:
    """Dynamically set the game window title."""
    ...

def set_window_title(title: str) -> None: ...

def toggleFullscreen() -> None:
    """Toggle the window between windowed and fullscreen modes."""
    ...

def toggle_fullscreen() -> None: ...

def getScreenSize() -> Tuple[int, int]:
    """Get the width and height of the window viewport."""
    ...

def get_screen_size() -> Tuple[int, int]: ...

def isNodeHovered(id: str) -> bool:
    """Check if the mouse cursor is hovering inside a node's bounding box."""
    ...

def is_node_hovered(id: str) -> bool: ...

def isNodeClicked(id: str) -> bool:
    """Check if a node was left-clicked in the current frame."""
    ...

def is_node_clicked(id: str) -> bool: ...

def setStyle(id: str, property_name: str, property_value: str) -> bool:
    """Dynamically set a style property value on a node."""
    ...

def set_style(id: str, property_name: str, property_value: str) -> bool: ...

def getStyle(id: str, property_name: str) -> str:
    """Retrieve the current value of a node's style property."""
    ...

def get_style(id: str, property_name: str) -> str: ...

def setCamera(target_x: float, target_y: float, offset_x: float, offset_y: float, zoom: float, rotation: float) -> None:
    """Set 2D camera translation, target, scale, and rotation parameters."""
    ...

def set_camera(target_x: float, target_y: float, offset_x: float, offset_y: float, zoom: float, rotation: float) -> None: ...

def shakeCamera(intensity: float, duration: float) -> None:
    """Trigger a screen shake camera effect with custom intensity and duration."""
    ...

def shake_camera(intensity: float, duration: float) -> None: ...

def spawnParticles(x: float, y: float, count: int, color_hex: str, speed: float, lifetime: float) -> None:
    """Spawn a particle explosion burst at coordinates."""
    ...

def spawn_particles(x: float, y: float, count: int, color_hex: str, speed: float, lifetime: float) -> None: ...

def playSynth(freq: float, duration: float, wave_type: int = 1, attack: float = 0.01, decay: float = 0.05, sustain: float = 0.5, release: float = 0.05) -> None:
    """Play a procedurally synthesized sound tone with ADSR envelope styling."""
    ...

def play_synth(freq: float, duration: float, wave_type: int = 1, attack: float = 0.01, decay: float = 0.05, sustain: float = 0.5, release: float = 0.05) -> None: ...

def registerConsoleCallback(callback: Callable[[str], str]) -> None:
    """Register a runtime developer console python command execution handler."""
    ...

def register_console_callback(callback: Callable[[str], str]) -> None: ...

def batchProcess(positions: Optional[Dict[str, Tuple[float, float]]] = None,
                 text_updates: Optional[Dict[str, str]] = None,
                 visibility_updates: Optional[Dict[str, bool]] = None,
                 collisions: Optional[List[Tuple[str, str]]] = None) -> Dict[Tuple[str, str], Union[bool, str, None]]:
    """
    Batches multiple node updates (positions, text, visibility) and collision checks
    into a single optimized C call to minimize Python-C roundtrip overhead.
    """
    ...

def batch_process(positions: Optional[Dict[str, Tuple[float, float]]] = None,
                  text_updates: Optional[Dict[str, str]] = None,
                  visibility_updates: Optional[Dict[str, bool]] = None,
                  collisions: Optional[List[Tuple[str, str]]] = None) -> Dict[Tuple[str, str], Union[bool, str, None]]: ...

def getLineNumber(id: str) -> int:
    """Get the parsed layout.html line number of a node."""
    ...

def get_line_number(id: str) -> int: ...

def getLayoutSize(id: str) -> Tuple[float, float]:
    """Get the computed layout box size (width, height) of a node."""
    ...

def get_layout_size(id: str) -> Tuple[float, float]: ...

def isVisible(id: str) -> bool:
    """Check if a node is currently visible."""
    ...

def is_visible(id: str) -> bool: ...

def getText(id: str) -> str:
    """Get the text content of a node."""
    ...

def get_text(id: str) -> str: ...
