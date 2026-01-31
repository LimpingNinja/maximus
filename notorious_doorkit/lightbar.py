"""
Lightbar menu system for classic BBS-style menus.
"""

from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple, Union

from .ansi import goto_xy, write, RESET, hide_cursor, show_cursor
from .input import RawInput, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_ESC, is_printable
from .style import resolve_style
from .theme import Theme, get_current_theme
from .utils import pad_string, visible_length


@dataclass
class LightbarItem:
    """
    Individual lightbar menu item with custom positioning.
    
    Attributes:
        text: Item text
        x: X position (1-indexed)
        y: Y position (1-indexed)
        width: Item width ("auto", "exact", or int)
        justify: Text justification ("left", "center", "right")
        hotkey: Custom hotkey (optional)
    """
    text: str
    x: int
    y: int
    width: Union[int, str] = "auto"
    justify: str = "left"
    hotkey: Optional[str] = None


class LightbarMenu:
    """
    Basic lightbar menu with uniform positioning.
    
    Example:
        >>> menu = LightbarMenu(
        ...     items=["Play Game", "View Scores", "Exit"],
        ...     x=10, y=15,
        ...     width="auto",
        ...     justify="center",
        ...     selected_color=CYAN,
        ...     normal_color=WHITE
        ... )
        >>> choice = menu.run()
    """
    
    def __init__(self,
                 items: List[str],
                 x: int = 1,
                 y: int = 1,
                 width: Union[int, str, None] = None,
                 justify: str = "left",
                 selected_color: Optional[str] = None,
                 normal_color: Optional[str] = None,
                 hotkeys: bool = True,
                 hotkey_format: Optional[str] = None,
                 hotkey_color: Optional[str] = None,
                 theme: Optional[Theme] = None,
                 margin_inner: int = 0,
                 wrap: bool = True):
        """
        Initialize lightbar menu.
        
        Args:
            items: Menu items
            x: X position (1-indexed)
            y: Y position (1-indexed)
            width: Bar width (None/"auto" = longest item, "exact" = exact width, int = fixed)
            justify: Text justification ("left", "center", "right")
            selected_color: ANSI color for selected item
            normal_color: ANSI color for normal items
            hotkeys: Enable first-letter hotkeys
            hotkey_format: Hotkey display format ("none", "(X)", "[X]", "underline")
            hotkey_color: ANSI color for hotkey character (empty = use item color)
            wrap: Wrap at top/bottom
        """
        self.items = items
        self.x = x
        self.y = y
        self.width = width
        self.justify = justify
        th = get_current_theme() if theme is None else theme
        self.selected_color = resolve_style(selected_color, th.colors.selected)
        self.normal_color = resolve_style(normal_color, th.colors.text)
        self.hotkeys = hotkeys
        self.hotkey_format = th.format.hotkey if hotkey_format is None else hotkey_format
        self.hotkey_color = resolve_style(hotkey_color, th.colors.key)
        self.margin_inner = max(0, int(margin_inner))
        self.wrap = wrap
        self.selected_index = 0
        
        # Parse hotkey markers and detect conflicts
        self._parse_hotkeys()
        
        # Calculate widths (using displayed text)
        if width is None or width == "auto":
            self.item_widths = [max(visible_length(self._display_text(i)[0]) for i in range(len(items))) + (2 * self.margin_inner)] * len(items)
        elif width == "exact":
            self.item_widths = [visible_length(self._display_text(i)[0]) + (2 * self.margin_inner) for i in range(len(items))]
        else:
            self.item_widths = [int(width)] * len(items)
            
    def _parse_hotkeys(self) -> None:
        """Parse hotkey markers from items and auto-detect unique hotkeys."""
        import re
        self._hotkey_map = {}  # index -> (hotkey_char, position_in_clean_text)
        self._clean_items = []  # items with {{X}} markers replaced by X
        used_hotkeys = set()
        
        # First pass: extract explicit {{X}} markers
        marker_re = re.compile(r'\{\{(.?)\}\}')
        for i, item in enumerate(self.items):
            m = marker_re.search(item)
            if not m:
                self._clean_items.append(item)
                continue

            hotkey_char = m.group(1)
            hotkey_lower = hotkey_char.lower()

            parts = []
            parts.append(item[:m.start()])
            hk_pos = len(''.join(parts))
            parts.append(hotkey_char)
            parts.append(item[m.end():])
            clean_text = ''.join(parts)
            self._clean_items.append(clean_text)

            if hotkey_char and hk_pos is not None:
                self._hotkey_map[i] = (hotkey_lower, hk_pos)
                used_hotkeys.add(hotkey_lower)
        
        # Second pass: auto-assign hotkeys for items without explicit markers
        if self.hotkeys:
            for i, item in enumerate(self._clean_items):
                if i in self._hotkey_map:
                    continue
                # Try each letter in the item
                for pos, char in enumerate(item):
                    if char.isalpha():
                        char_lower = char.lower()
                        if char_lower not in used_hotkeys:
                            self._hotkey_map[i] = (char_lower, pos)
                            used_hotkeys.add(char_lower)
                            break
    
    def _display_text(self, index: int) -> tuple[str, Optional[int]]:
        item = self._clean_items[index]
        if not self.hotkeys or not item or self.hotkey_format == "none":
            return item, None

        if index not in self._hotkey_map:
            return item, None

        _hk_lower, hk_pos = self._hotkey_map[index]
        if hk_pos < 0 or hk_pos >= len(item):
            return item, None

        if self.hotkey_format == "[X]":
            return item[:hk_pos] + "[" + item[hk_pos] + "]" + item[hk_pos + 1:], hk_pos + 1
        if self.hotkey_format == "(X)":
            return item[:hk_pos] + "(" + item[hk_pos] + ")" + item[hk_pos + 1:], hk_pos + 1
        if self.hotkey_format == "underline":
            return item, hk_pos
        return item, hk_pos
    
    def draw_item(self, index: int, selected: bool) -> None:
        """
        Draw a single menu item.
        
        Args:
            index: Item index
            selected: Whether item is selected
        """
        if index < 0 or index >= len(self.items):
            return
            
        item_width = self.item_widths[index]
        color = self.selected_color if selected else self.normal_color
        hk_color = self.hotkey_color if self.hotkey_color else ""
        
        # Position cursor
        goto_xy(self.x, self.y + index)
        
        text, hk_pos = self._display_text(index)

        content_width = max(0, item_width - (2 * self.margin_inner))

        if visible_length(text) > content_width:
            text = text[:content_width]
            if hk_pos is not None and hk_pos >= content_width:
                hk_pos = None

        pad_needed = content_width - visible_length(text)
        if pad_needed < 0:
            pad_needed = 0

        if self.justify == "right":
            left_pad = pad_needed
        elif self.justify == "center":
            left_pad = pad_needed // 2
        else:
            left_pad = 0
        right_pad = pad_needed - left_pad

        write(color)
        if self.margin_inner:
            write(" " * self.margin_inner)
        if left_pad:
            write(" " * left_pad)

        if hk_pos is None or hk_pos < 0 or hk_pos >= len(text) or not hk_color:
            write(text)
        else:
            if self.hotkey_format == "underline":
                write(text[:hk_pos])
                write(f"{hk_color}\x1b[4m{text[hk_pos]}\x1b[24m\x1b[22m{color}")
                write(text[hk_pos + 1:])
            else:
                write(text[:hk_pos])
                write(f"{hk_color}{text[hk_pos]}\x1b[22m{color}")
                write(text[hk_pos + 1:])

        if right_pad:
            write(" " * right_pad)
        if self.margin_inner:
            write(" " * self.margin_inner)
        write(RESET)
        
    def redraw(self) -> None:
        """Redraw entire menu."""
        for i in range(len(self.items)):
            self.draw_item(i, i == self.selected_index)
            
    def run(self) -> Optional[int]:
        """
        Display menu and wait for selection.
        
        Returns:
            Selected index or None on ESC
        """
        hide_cursor()
        self.redraw()
        
        with RawInput() as inp:
            while True:
                key = inp.get_key()
                
                if key == KEY_ENTER:
                    show_cursor()
                    return self.selected_index
                    
                elif key == KEY_ESC:
                    show_cursor()
                    return None
                    
                elif key == KEY_UP:
                    old_index = self.selected_index
                    self.selected_index -= 1
                    if self.selected_index < 0:
                        self.selected_index = len(self.items) - 1 if self.wrap else 0
                    if old_index != self.selected_index:
                        self.draw_item(old_index, False)
                        self.draw_item(self.selected_index, True)
                        
                elif key == KEY_DOWN:
                    old_index = self.selected_index
                    self.selected_index += 1
                    if self.selected_index >= len(self.items):
                        self.selected_index = 0 if self.wrap else len(self.items) - 1
                    if old_index != self.selected_index:
                        self.draw_item(old_index, False)
                        self.draw_item(self.selected_index, True)
                        
                elif self.hotkeys and key and len(key) == 1 and is_printable(key):
                    # Check for hotkey match
                    key_lower = key.lower()
                    for i in range(len(self.items)):
                        if i in self._hotkey_map:
                            hotkey_char, _ = self._hotkey_map[i]
                            if hotkey_char == key_lower:
                                show_cursor()
                                return i


class AdvancedLightbarMenu:
    """
    Advanced lightbar menu with individual item positioning.
    
    Example:
        >>> items = [
        ...     LightbarItem("New Game", x=10, y=10, width=20, justify="center"),
        ...     LightbarItem("Load Game", x=10, y=12, width=20, justify="center"),
        ...     LightbarItem("Options", x=40, y=10, width=15, justify="left"),
        ...     LightbarItem("Quit", x=40, y=12, width=15, justify="left"),
        ... ]
        >>> menu = AdvancedLightbarMenu(items, selected_color=YELLOW, normal_color=WHITE)
        >>> choice = menu.run()
    """
    
    def __init__(self,
                 items: List[LightbarItem],
                 selected_color: Optional[str] = None,
                 normal_color: Optional[str] = None,
                 hotkeys: bool = True,
                 hotkey_format: Optional[str] = None,
                 hotkey_color: Optional[str] = None,
                 theme: Optional[Theme] = None,
                 margin_inner: int = 0,
                 wrap: bool = True):
        """
        Initialize advanced lightbar menu.
        
        Args:
            items: List of LightbarItem objects
            selected_color: ANSI color for selected item
            normal_color: ANSI color for normal items
            hotkeys: Enable hotkeys
            hotkey_format: Hotkey display format ("none", "(X)", "[X]", "underline")
            hotkey_color: ANSI color for hotkey character (empty = use item color)
            wrap: Wrap at top/bottom
        """
        self.items = items
        th = get_current_theme() if theme is None else theme
        self.selected_color = resolve_style(selected_color, th.colors.selected)
        self.normal_color = resolve_style(normal_color, th.colors.text)
        self.hotkeys = hotkeys
        self.hotkey_format = th.format.hotkey if hotkey_format is None else hotkey_format
        self.hotkey_color = resolve_style(hotkey_color, th.colors.key)
        self.margin_inner = max(0, int(margin_inner))
        self.wrap = wrap
        self.selected_index = 0

        self._hotkey_map: Dict[int, Tuple[str, int]] = {}
        self._parse_hotkeys()
        
        # Calculate widths for each item
        for i, item in enumerate(self.items):
            if item.width == "auto" or item.width == "exact":
                disp, _hk_pos = self._display_text(i)
                item.width = visible_length(disp) + (2 * self.margin_inner)
            # else: already an int
            
    def _parse_hotkeys(self) -> None:
        import re

        used_hotkeys = set()
        marker_re = re.compile(r'\{\{(.?)\}\}')

        # First pass: explicit {{X}} markers (keep the letter, drop braces)
        for i, item in enumerate(self.items):
            m = marker_re.search(item.text)
            if not m:
                continue

            hotkey_char = m.group(1)
            hotkey_lower = hotkey_char.lower()
            before = item.text[:m.start()]
            hk_pos = len(before)
            after = item.text[m.end():]
            item.text = before + hotkey_char + after
            self._hotkey_map[i] = (hotkey_lower, hk_pos)
            used_hotkeys.add(hotkey_lower)

        # Second pass: if explicit item.hotkey is set, prefer it
        for i, item in enumerate(self.items):
            if not item.hotkey:
                continue
            hk_lower = item.hotkey[0].lower()
            pos = item.text.lower().find(hk_lower)
            if pos != -1:
                # If already mapped from {{}} and different, keep existing mapping
                if i not in self._hotkey_map:
                    self._hotkey_map[i] = (hk_lower, pos)
                    used_hotkeys.add(hk_lower)

        # Third pass: auto-assign unique hotkeys
        if self.hotkeys:
            for i, item in enumerate(self.items):
                if i in self._hotkey_map:
                    continue
                for pos, ch in enumerate(item.text):
                    if ch.isalpha():
                        ch_lower = ch.lower()
                        if ch_lower not in used_hotkeys:
                            self._hotkey_map[i] = (ch_lower, pos)
                            used_hotkeys.add(ch_lower)
                            break

    def _display_text(self, index: int) -> tuple[str, Optional[int]]:
        text = self.items[index].text
        if not self.hotkeys or not text or self.hotkey_format == "none":
            return text, None

        if index not in self._hotkey_map:
            return text, None

        _hk_lower, hk_pos = self._hotkey_map[index]
        if hk_pos < 0 or hk_pos >= len(text):
            return text, None

        if self.hotkey_format == "[X]":
            return text[:hk_pos] + "[" + text[hk_pos] + "]" + text[hk_pos + 1:], hk_pos + 1
        if self.hotkey_format == "(X)":
            return text[:hk_pos] + "(" + text[hk_pos] + ")" + text[hk_pos + 1:], hk_pos + 1
        if self.hotkey_format == "underline":
            return text, hk_pos
        return text, hk_pos
    
    def draw_item(self, index: int, selected: bool) -> None:
        """
        Draw a single menu item.
        
        Args:
            index: Item index
            selected: Whether item is selected
        """
        if index < 0 or index >= len(self.items):
            return
            
        item = self.items[index]
        color = self.selected_color if selected else self.normal_color
        hk_color = self.hotkey_color if self.hotkey_color else ""
        
        # Position cursor
        goto_xy(item.x, item.y)

        item_width = item.width if isinstance(item.width, int) else visible_length(item.text)
        text, hk_pos = self._display_text(index)

        content_width = max(0, item_width - (2 * self.margin_inner))

        if visible_length(text) > content_width:
            text = text[:content_width]
            if hk_pos is not None and hk_pos >= content_width:
                hk_pos = None

        pad_needed = content_width - visible_length(text)
        if pad_needed < 0:
            pad_needed = 0

        if item.justify == "right":
            left_pad = pad_needed
        elif item.justify == "center":
            left_pad = pad_needed // 2
        else:
            left_pad = 0
        right_pad = pad_needed - left_pad

        write(color)
        if self.margin_inner:
            write(" " * self.margin_inner)
        if left_pad:
            write(" " * left_pad)

        if hk_pos is None or hk_pos < 0 or hk_pos >= len(text) or not hk_color:
            write(text)
        else:
            if self.hotkey_format == "underline":
                write(text[:hk_pos])
                write(f"{hk_color}\x1b[4m{text[hk_pos]}\x1b[24m\x1b[22m{color}")
                write(text[hk_pos + 1:])
            else:
                write(text[:hk_pos])
                write(f"{hk_color}{text[hk_pos]}\x1b[22m{color}")
                write(text[hk_pos + 1:])

        if right_pad:
            write(" " * right_pad)
        if self.margin_inner:
            write(" " * self.margin_inner)
        write(RESET)
        
    def redraw(self) -> None:
        """Redraw entire menu."""
        for i in range(len(self.items)):
            self.draw_item(i, i == self.selected_index)

    def _item_center_x(self, index: int) -> float:
        item = self.items[index]
        item_width = item.width if isinstance(item.width, int) else visible_length(item.text)
        return float(item.x) + (float(item_width) - 1.0) / 2.0

    def _find_lr_neighbor(self, direction: str) -> Optional[int]:
        """Find next item index in the given horizontal direction ("left" or "right")."""
        if not self.items:
            return None

        cur_idx = self.selected_index
        cur_item = self.items[cur_idx]
        cur_cx = self._item_center_x(cur_idx)
        cur_y = cur_item.y

        candidates = []
        for i in range(len(self.items)):
            if i == cur_idx:
                continue
            cx = self._item_center_x(i)
            dy = abs(self.items[i].y - cur_y)
            dx = cx - cur_cx
            if direction == "right" and dx > 0:
                candidates.append((dx, dy, i))
            elif direction == "left" and dx < 0:
                candidates.append((-dx, dy, i))

        if candidates:
            candidates.sort(key=lambda t: (t[0], t[1], t[2]))
            return candidates[0][2]

        if not self.wrap:
            return None

        # Wrap behavior: jump to the extreme opposite side, picking the closest row.
        if direction == "right":
            extreme = min(self._item_center_x(i) for i in range(len(self.items)) if i != cur_idx)
            pool = [i for i in range(len(self.items)) if i != cur_idx and self._item_center_x(i) == extreme]
        else:
            extreme = max(self._item_center_x(i) for i in range(len(self.items)) if i != cur_idx)
            pool = [i for i in range(len(self.items)) if i != cur_idx and self._item_center_x(i) == extreme]

        if not pool:
            return None

        pool.sort(key=lambda i: (abs(self.items[i].y - cur_y), abs(self._item_center_x(i) - cur_cx), i))
        return pool[0]
            
    def run(self) -> Optional[int]:
        """
        Display menu and wait for selection.
        
        Returns:
            Selected index or None on ESC
        """
        hide_cursor()
        self.redraw()
        
        with RawInput() as inp:
            while True:
                key = inp.get_key()
                
                if key == KEY_ENTER:
                    show_cursor()
                    return self.selected_index
                    
                elif key == KEY_ESC:
                    show_cursor()
                    return None
                    
                elif key == KEY_UP:
                    old_index = self.selected_index
                    self.selected_index -= 1
                    if self.selected_index < 0:
                        self.selected_index = len(self.items) - 1 if self.wrap else 0
                    if old_index != self.selected_index:
                        self.draw_item(old_index, False)
                        self.draw_item(self.selected_index, True)
                        
                elif key == KEY_DOWN:
                    old_index = self.selected_index
                    self.selected_index += 1
                    if self.selected_index >= len(self.items):
                        self.selected_index = 0 if self.wrap else len(self.items) - 1
                    if old_index != self.selected_index:
                        self.draw_item(old_index, False)
                        self.draw_item(self.selected_index, True)

                elif key == KEY_LEFT:
                    old_index = self.selected_index
                    nxt = self._find_lr_neighbor("left")
                    if nxt is not None:
                        self.selected_index = nxt
                    if old_index != self.selected_index:
                        self.draw_item(old_index, False)
                        self.draw_item(self.selected_index, True)

                elif key == KEY_RIGHT:
                    old_index = self.selected_index
                    nxt = self._find_lr_neighbor("right")
                    if nxt is not None:
                        self.selected_index = nxt
                    if old_index != self.selected_index:
                        self.draw_item(old_index, False)
                        self.draw_item(self.selected_index, True)
                        
                elif self.hotkeys and key and len(key) == 1 and is_printable(key):
                    key_lower = key.lower()
                    for i in range(len(self.items)):
                        if i in self._hotkey_map:
                            hk_char, _hk_pos = self._hotkey_map[i]
                            if hk_char == key_lower:
                                show_cursor()
                                return i
