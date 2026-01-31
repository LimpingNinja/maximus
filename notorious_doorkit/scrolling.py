"""
Scrolling text regions and viewers for notorious_doorkit.

This module provides:
- ScrollingRegion: Chat/log viewport with auto-scroll and manual navigation
- TextBufferViewer: Interactive ANSI text viewer (like 'less')
"""

import sys
import select
from typing import List, Optional
from .ansi import goto_xy, write, clear_to_eol, strip_ansi


class ScrollingRegion:
    """
    A viewport for displaying scrolling text (chat, logs, feeds).
    
    The region displays the last N lines of a buffer within a fixed screen area.
    New lines auto-scroll to bottom unless user has scrolled up to read history.
    
    Coordinates are 1-indexed and inclusive.
    
    Example:
        >>> region = ScrollingRegion(x=5, y=10, width=40, height=10, max_lines=500)
        >>> region.append("Welcome to the chat!")
        >>> region.append("User joined the room")
        >>> region.render()
        >>> region.scroll_up()  # User wants to read history
        >>> region.render()
    """
    
    def __init__(
        self,
        x: int,
        y: int,
        width: int,
        height: int,
        max_lines: int = 1000,
        show_scrollbar: bool = False,
    ):
        """
        Initialize scrolling region.
        
        Args:
            x: Left edge (1-indexed)
            y: Top edge (1-indexed)
            width: Width of text viewport
            height: Height of viewport
            max_lines: Maximum lines to keep in buffer (oldest dropped)
            show_scrollbar: If True, draw scrollbar at x+width (outside text area)
        """
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.max_lines = max_lines
        self.show_scrollbar = show_scrollbar
        
        self._lines: List[str] = []
        self._view_top = 0
        self._at_bottom = True  # Auto-scroll when True
        self._last_thumb_top = -1
        self._last_thumb_len = -1
        
    @property
    def text_width(self) -> int:
        """Usable width for text."""
        return self.width
    
    def append(self, text: str) -> None:
        """
        Append text to the buffer.
        
        Args:
            text: Text to append (can contain newlines and ANSI codes)
        """
        lines = text.split('\n')
        for line in lines:
            # Wrap long lines to fit viewport width
            wrapped = self._wrap_line(line)
            self._lines.extend(wrapped)
        
        # Trim buffer if it exceeds max_lines
        if len(self._lines) > self.max_lines:
            excess = len(self._lines) - self.max_lines
            self._lines = self._lines[excess:]
            # Adjust view_top if we dropped lines
            self._view_top = max(0, self._view_top - excess)
        
        # Auto-scroll to bottom if we were at bottom
        if self._at_bottom:
            self.scroll_to_bottom()
    
    def _wrap_line(self, line: str) -> List[str]:
        """
        Wrap a line to fit text_width, preserving ANSI codes.
        
        Args:
            line: Line to wrap (may contain ANSI codes)
            
        Returns:
            List of wrapped lines
        """
        if not line:
            return ['']
        
        # Simple wrapping: split on text_width visible characters
        # This is naive but works for most cases
        visible_len = len(strip_ansi(line))
        if visible_len <= self.text_width:
            return [line]
        
        # For now, just truncate long lines
        # TODO: Proper ANSI-aware wrapping
        return [line[:self.text_width]]
    
    def scroll_up(self, lines: int = 1) -> None:
        """Scroll viewport up (toward older messages)."""
        if len(self._lines) <= self.height:
            return
        
        self._view_top = max(0, self._view_top - lines)
        self._at_bottom = (self._view_top >= len(self._lines) - self.height)
    
    def scroll_down(self, lines: int = 1) -> None:
        """Scroll viewport down (toward newer messages)."""
        if len(self._lines) <= self.height:
            return
        
        max_top = max(0, len(self._lines) - self.height)
        self._view_top = min(max_top, self._view_top + lines)
        self._at_bottom = (self._view_top >= max_top)
    
    def page_up(self) -> None:
        """Scroll up by one page."""
        self.scroll_up(self.height)
    
    def page_down(self) -> None:
        """Scroll down by one page."""
        self.scroll_down(self.height)
    
    def scroll_to_top(self) -> None:
        """Jump to the oldest message."""
        self._view_top = 0
        self._at_bottom = False
    
    def scroll_to_bottom(self) -> None:
        """Jump to the newest message."""
        if len(self._lines) <= self.height:
            self._view_top = 0
        else:
            self._view_top = len(self._lines) - self.height
        self._at_bottom = True
    
    def render(self) -> None:
        """Render the current viewport to the screen."""
        text_width = self.text_width
        
        for row in range(self.height):
            idx = self._view_top + row
            if 0 <= idx < len(self._lines):
                line = self._lines[idx]
                # Truncate to text_width visible chars
                visible = strip_ansi(line)
                if len(visible) > text_width:
                    # Naive truncation
                    line = line[:text_width]
                # Pad to text_width
                visible_len = len(strip_ansi(line))
                padding = ' ' * max(0, text_width - visible_len)
                display = line + padding
            else:
                display = ' ' * text_width
            
            goto_xy(self.x, self.y + row)
            write(display)
            if not self.show_scrollbar:
                clear_to_eol()
        
        if self.show_scrollbar:
            self._render_scrollbar()
    
    def _render_scrollbar(self) -> None:
        """Render scrollbar outside the text viewport (at x + width).
        
        Only redraws cells that changed since last render.
        """
        sb_x = self.x + self.width
        
        if len(self._lines) <= self.height:
            # No scrolling needed, show full bar
            thumb_top = 0
            thumb_len = self.height
        else:
            # Calculate thumb position and size
            max_top = max(1, len(self._lines) - self.height)
            thumb_len = max(1, int(round((self.height * self.height) / max(1, len(self._lines)))))
            thumb_len = min(self.height, thumb_len)
            usable = max(1, self.height - thumb_len)
            thumb_top = int(round((self._view_top / max_top) * usable))
        
        # Only redraw if thumb position/size changed
        if thumb_top == self._last_thumb_top and thumb_len == self._last_thumb_len:
            return
        
        # Calculate which rows need updating
        old_top = self._last_thumb_top
        old_len = self._last_thumb_len
        
        if old_top < 0:
            # First render - draw everything
            for row in range(self.height):
                ch = '█' if thumb_top <= row < (thumb_top + thumb_len) else '░'
                goto_xy(sb_x, self.y + row)
                write(ch)
        else:
            # Only redraw changed cells
            old_end = old_top + old_len
            new_end = thumb_top + thumb_len
            
            # Determine range that needs redrawing
            min_row = min(old_top, thumb_top)
            max_row = max(old_end, new_end)
            
            for row in range(min_row, max_row):
                if row >= self.height:
                    break
                # Check if this cell changed
                was_thumb = old_top <= row < old_end
                is_thumb = thumb_top <= row < new_end
                if was_thumb != is_thumb:
                    ch = '█' if is_thumb else '░'
                    goto_xy(sb_x, self.y + row)
                    write(ch)
        
        self._last_thumb_top = thumb_top
        self._last_thumb_len = thumb_len
    
    def clear(self) -> None:
        """Clear the buffer and reset viewport."""
        self._lines.clear()
        self._view_top = 0
        self._at_bottom = True


class TextBufferViewer:
    """
    Interactive ANSI text viewer (like 'less').
    
    Displays a large text buffer in a viewport with navigation controls.
    Read-only, supports ANSI color codes.
    
    Controls:
    - arrows: line up/down, char left/right
    - PgUp/PgDn or Ctrl+U/Ctrl+D: page up/down
    - Home/End or Ctrl+H/Ctrl+E: jump to top/bottom
    - ESC or 'q': exit viewer
    
    Example:
        >>> viewer = TextBufferViewer(x=1, y=1, width=80, height=23)
        >>> with open('large_file.txt') as f:
        ...     viewer.set_text(f.read())
        >>> viewer.view()  # Blocks until user presses ESC
    """
    
    def __init__(
        self,
        x: int,
        y: int,
        width: int,
        height: int,
        show_status: bool = True,
        show_scrollbar: bool = True,
    ):
        """
        Initialize text buffer viewer.
        
        Args:
            x: Left edge (1-indexed)
            y: Top edge (1-indexed)
            width: Width of viewport
            height: Height of viewport (includes status line if enabled)
            show_status: Show status line at bottom
            show_scrollbar: Show scrollbar on right edge
        """
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.show_status = show_status
        self.show_scrollbar = show_scrollbar
        
        self._lines: List[str] = []
        self._view_top = 0
        self._view_left = 0
        self._last_thumb_top = -1
        self._last_thumb_len = -1
    
    @property
    def text_height(self) -> int:
        """Usable height for text (accounting for status line)."""
        return self.height - 1 if self.show_status else self.height
    
    @property
    def text_width(self) -> int:
        """Usable width for text."""
        return self.width
    
    def set_text(self, text: str) -> None:
        """
        Load text into the buffer.
        
        Args:
            text: Text to display (can contain newlines and ANSI codes)
        """
        self._lines = text.split('\n') if text else ['']
        self._view_top = 0
        self._view_left = 0
    
    def scroll_up(self, lines: int = 1) -> None:
        """Scroll viewport up."""
        self._view_top = max(0, self._view_top - lines)
    
    def scroll_down(self, lines: int = 1) -> None:
        """Scroll viewport down."""
        max_top = max(0, len(self._lines) - self.text_height)
        self._view_top = min(max_top, self._view_top + lines)
    
    def scroll_left(self, cols: int = 1) -> None:
        """Scroll viewport left."""
        self._view_left = max(0, self._view_left - cols)
    
    def scroll_right(self, cols: int = 1) -> None:
        """Scroll viewport right."""
        # Allow scrolling right up to 200 columns
        self._view_left = min(200, self._view_left + cols)
    
    def page_up(self) -> None:
        """Scroll up by one page."""
        self.scroll_up(self.text_height)
    
    def page_down(self) -> None:
        """Scroll down by one page."""
        self.scroll_down(self.text_height)
    
    def scroll_to_top(self) -> None:
        """Jump to the top of the buffer."""
        self._view_top = 0
    
    def scroll_to_bottom(self) -> None:
        """Jump to the bottom of the buffer."""
        max_top = max(0, len(self._lines) - self.text_height)
        self._view_top = max_top
    
    def render(self) -> None:
        """Render the current viewport to the screen."""
        text_width = self.text_width
        text_height = self.text_height
        
        for row in range(text_height):
            idx = self._view_top + row
            if 0 <= idx < len(self._lines):
                line = self._lines[idx]
                # Handle horizontal scrolling (naive: just skip chars)
                visible = strip_ansi(line)
                if self._view_left < len(visible):
                    # Slice visible portion
                    display = visible[self._view_left:self._view_left + text_width]
                    # Pad to text_width
                    display = display + (' ' * max(0, text_width - len(display)))
                else:
                    display = ' ' * text_width
            else:
                display = ' ' * text_width
            
            goto_xy(self.x, self.y + row)
            write(display)
            if not self.show_scrollbar:
                clear_to_eol()
        
        if self.show_scrollbar:
            self._render_scrollbar()
        
        if self.show_status:
            self._render_status()
    
    def _render_scrollbar(self) -> None:
        """Render scrollbar outside the text viewport (at x + width).
        
        Only redraws cells that changed since last render.
        """
        text_height = self.text_height
        sb_x = self.x + self.width
        
        if len(self._lines) <= text_height:
            # No scrolling needed
            thumb_top = 0
            thumb_len = text_height
        else:
            # Calculate thumb position and size
            max_top = max(1, len(self._lines) - text_height)
            thumb_len = max(1, int(round((text_height * text_height) / max(1, len(self._lines)))))
            thumb_len = min(text_height, thumb_len)
            usable = max(1, text_height - thumb_len)
            thumb_top = int(round((self._view_top / max_top) * usable))
        
        # Only redraw if thumb position/size changed
        if thumb_top == self._last_thumb_top and thumb_len == self._last_thumb_len:
            return
        
        # Calculate which rows need updating
        old_top = self._last_thumb_top
        old_len = self._last_thumb_len
        
        if old_top < 0:
            # First render - draw everything
            for row in range(text_height):
                ch = '█' if thumb_top <= row < (thumb_top + thumb_len) else '░'
                goto_xy(sb_x, self.y + row)
                write(ch)
        else:
            # Only redraw changed cells
            old_end = old_top + old_len
            new_end = thumb_top + thumb_len
            
            # Determine range that needs redrawing
            min_row = min(old_top, thumb_top)
            max_row = max(old_end, new_end)
            
            for row in range(min_row, max_row):
                if row >= text_height:
                    break
                # Check if this cell changed
                was_thumb = old_top <= row < old_end
                is_thumb = thumb_top <= row < new_end
                if was_thumb != is_thumb:
                    ch = '█' if is_thumb else '░'
                    goto_xy(sb_x, self.y + row)
                    write(ch)
        
        self._last_thumb_top = thumb_top
        self._last_thumb_len = thumb_len
    
    def _render_status(self) -> None:
        """Render status line at bottom."""
        total_lines = len(self._lines)
        current_line = self._view_top + 1
        if total_lines > 0:
            percent = int((self._view_top / max(1, total_lines - 1)) * 100)
        else:
            percent = 0
        
        status = f" Line {current_line}/{total_lines} ({percent}%) "
        status = status[:self.width]
        status = status + (' ' * max(0, self.width - len(status)))
        
        goto_xy(self.x, self.y + self.height - 1)
        write(status)
        clear_to_eol()
    
    def view(self) -> None:
        """
        Enter interactive viewing mode (blocks until user exits).
        
        Returns when user presses ESC or 'q'.
        """
        from .ansi import hide_cursor, show_cursor
        from .input import RawInput, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT
        from .input import KEY_PAGEUP, KEY_PAGEDOWN, KEY_HOME, KEY_END, KEY_ESC
        
        hide_cursor()
        try:
            self.render()
            
            with RawInput(extended_keys=True) as inp:
                while True:
                    key = inp.get_key()
                    
                    if key == KEY_ESC or key == 'q':
                        break
                    
                    elif key == KEY_UP:
                        self.scroll_up()
                        self.render()
                    
                    elif key == KEY_DOWN:
                        self.scroll_down()
                        self.render()
                    
                    elif key == KEY_LEFT:
                        self.scroll_left()
                        self.render()
                    
                    elif key == KEY_RIGHT:
                        self.scroll_right()
                        self.render()
                    
                    elif key == KEY_PAGEUP or key == '\x15':  # Ctrl+U
                        self.page_up()
                        self.render()
                    
                    elif key == KEY_PAGEDOWN or key == '\x04':  # Ctrl+D
                        self.page_down()
                        self.render()
                    
                    elif key == KEY_HOME or key == '\x08':  # Ctrl+H
                        self.scroll_to_top()
                        self.render()
                    
                    elif key == KEY_END or key == '\x05':  # Ctrl+E
                        self.scroll_to_bottom()
                        self.render()
        finally:
            show_cursor()
