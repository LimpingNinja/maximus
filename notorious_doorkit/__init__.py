"""
NotoriousPTY Door Kit - A Python library for building BBS doors on NotoriousPTY.

This library provides high-level abstractions for:
- ANSI terminal control and color
- Raw mode input handling with arrow key support
- Lightbar menu systems
- CP437 file display
- Screen block management
"""

__version__ = "0.1.0"
__author__ = "NotoriousPTY Project"

from .door import Door, DoorSession, TerminalInfo
from .ansi import (
    RESET, BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE,
    BRIGHT_BLACK, BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW,
    BRIGHT_BLUE, BRIGHT_MAGENTA, BRIGHT_CYAN, BRIGHT_WHITE,
    BG_BLACK, BG_RED, BG_GREEN, BG_YELLOW, BG_BLUE, BG_MAGENTA, BG_CYAN, BG_WHITE,
    BG_BRIGHT_BLACK, BG_BRIGHT_RED, BG_BRIGHT_GREEN, BG_BRIGHT_YELLOW,
    BG_BRIGHT_BLUE, BG_BRIGHT_MAGENTA, BG_BRIGHT_CYAN, BG_BRIGHT_WHITE,
    BOLD, DIM, ITALIC, UNDERLINE, BLINK, REVERSE, HIDDEN,
    goto_xy, set_cursor, clear_screen, clear_line, write, writeln, write_at, strip_ansi, play_ansi_music, play_music
)
from .ansi import get_cursor as get_cursor_position
from .input import RawInput, RegionEditor, KEY_ENTER, KEY_ESC, KEY_BACKSPACE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, GETIN_NORMAL, GETIN_RAW, GETIN_RAWCTRL, NO_TIMEOUT, KEYCODE_F1, KEYCODE_F2, KEYCODE_F3, KEYCODE_F4, KEYCODE_F5, KEYCODE_F6, KEYCODE_F7, KEYCODE_F8, KEYCODE_F9, KEYCODE_F10, KEYCODE_F11, KEYCODE_F12, KEYCODE_UP, KEYCODE_DOWN, KEYCODE_LEFT, KEYCODE_RIGHT, KEYCODE_HOME, KEYCODE_END, KEYCODE_PGUP, KEYCODE_PGDN, KEYCODE_INSERT, KEYCODE_DELETE, KEYCODE_SHIFTTAB, InputEvent, get_answer, hotkey_menu, key_pending, clear_keybuffer, get_input, input_str, edit_str, detect_capabilities_probe, detect_window_size_probe, prompt_graphics_mode
from .display import display_file, display_file_best_match, display_file_paged, display_block, more_prompt
from .lightbar import LightbarMenu, AdvancedLightbarMenu, LightbarItem
from .boxes import ascii_box, ansi_box
from .forms import InputField, MultiLineEditor, FormField, AdvancedForm
from .dropfiles import (
    DropfileType,
    DropfileUser,
    DropfileInfo,
    DropfileSession,
    load_dropfile,
    write_door_sys,
)
from .config import get_config, configure
from .runtime import (
    get_runtime,
    get_runtime_config,
    get_session,
    get_dropfile,
    get_terminal,
    get_username,
    get_screen_size,
    get_lnwp,
)
from .scrolling import ScrollingRegion, TextBufferViewer
from .text import printf, sprintf, disp_str, disp, disp_emu, repeat, putch, Attr, set_attrib, get_attrib
from .screen import ShadowScreen, ScreenBlock, ScreenSnapshot, WindowHandle, get_screen, set_screen, save_screen, restore_screen, gettext, puttext, window_create, window_remove, get_cursor, scroll, scroll_region, SCROLL_NORMAL, SCROLL_NO_CLEAR
from .style import resolve_style
from .theme import Theme, ThemeColors, ThemeFormat, build_theme, set_current_theme, get_current_theme, clear_current_theme

__all__ = [
    "Door",
    "DoorSession",
    "TerminalInfo",
    "RESET", "BLACK", "RED", "GREEN", "YELLOW", "BLUE", "MAGENTA", "CYAN", "WHITE",
    "BRIGHT_BLACK", "BRIGHT_RED", "BRIGHT_GREEN", "BRIGHT_YELLOW",
    "BRIGHT_BLUE", "BRIGHT_MAGENTA", "BRIGHT_CYAN", "BRIGHT_WHITE",
    "BG_BLACK", "BG_RED", "BG_GREEN", "BG_YELLOW", "BG_BLUE", "BG_MAGENTA", "BG_CYAN", "BG_WHITE",
    "BG_BRIGHT_BLACK", "BG_BRIGHT_RED", "BG_BRIGHT_GREEN", "BG_BRIGHT_YELLOW",
    "BG_BRIGHT_BLUE", "BG_BRIGHT_MAGENTA", "BG_BRIGHT_CYAN", "BG_BRIGHT_WHITE",
    "BOLD", "DIM", "ITALIC", "UNDERLINE", "BLINK", "REVERSE", "HIDDEN",
    "goto_xy", "set_cursor", "clear_screen", "clear_line", "write", "writeln", "write_at", "strip_ansi",
    "play_ansi_music",
    "play_music",
    "RawInput", "RegionEditor", "KEY_ENTER", "KEY_ESC", "KEY_BACKSPACE", "KEY_UP", "KEY_DOWN", "KEY_LEFT", "KEY_RIGHT",
    "GETIN_NORMAL", "GETIN_RAW", "GETIN_RAWCTRL",
    "NO_TIMEOUT",
    "KEYCODE_F1", "KEYCODE_F2", "KEYCODE_F3", "KEYCODE_F4", "KEYCODE_F5", "KEYCODE_F6", "KEYCODE_F7", "KEYCODE_F8", "KEYCODE_F9", "KEYCODE_F10", "KEYCODE_F11", "KEYCODE_F12",
    "KEYCODE_UP", "KEYCODE_DOWN", "KEYCODE_LEFT", "KEYCODE_RIGHT", "KEYCODE_HOME", "KEYCODE_END", "KEYCODE_PGUP", "KEYCODE_PGDN", "KEYCODE_INSERT", "KEYCODE_DELETE", "KEYCODE_SHIFTTAB",
    "display_file", "display_file_paged", "display_block", "more_prompt",
    "display_file_best_match",
    "ascii_box", "ansi_box",
    "LightbarMenu", "AdvancedLightbarMenu", "LightbarItem",
    "InputField", "MultiLineEditor", "FormField", "AdvancedForm",
    "DropfileType", "DropfileUser", "DropfileInfo", "DropfileSession", "load_dropfile", "write_door_sys",
    "get_config", "configure",
    "get_runtime",
    "get_runtime_config",
    "get_session",
    "get_dropfile",
    "get_terminal",
    "get_username",
    "get_screen_size",
    "get_lnwp",
    "ScrollingRegion", "TextBufferViewer",
    "printf",
    "sprintf",
    "disp_str",
    "disp",
    "disp_emu",
    "Attr",
    "set_attrib",
    "get_attrib",
    "repeat",
    "putch",
    "InputEvent",
    "get_answer",
    "detect_capabilities_probe",
    "detect_window_size_probe",
    "prompt_graphics_mode",
    "hotkey_menu",
    "key_pending",
    "clear_keybuffer",
    "get_input",
    "input_str",
    "edit_str",
    "ShadowScreen",
    "ScreenBlock",
    "ScreenSnapshot",
    "WindowHandle",
    "get_screen",
    "set_screen",
    "save_screen",
    "restore_screen",
    "gettext",
    "puttext",
    "window_create",
    "window_remove",
    "get_cursor",
    "scroll",
    "scroll_region",
    "SCROLL_NORMAL",
    "SCROLL_NO_CLEAR",
    "get_cursor_position",
    "resolve_style",
    "Theme",
    "ThemeColors",
    "ThemeFormat",
    "build_theme",
    "set_current_theme",
    "get_current_theme",
    "clear_current_theme",
]
