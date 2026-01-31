"""
Input forms and field editors.
"""

from dataclasses import dataclass, field
from typing import Callable, Dict, List, Optional, Tuple

from .ansi import RESET, write, writeln, goto_xy, clear_to_eol, hide_cursor, show_cursor
from .input import RawInput, KEY_ENTER, KEY_ESC, KEY_BACKSPACE, KEY_DELETE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_HOME, KEY_END, is_printable
from .style import resolve_style
from .theme import Theme, get_current_theme
from .utils import pad_string, visible_length


class InputField:
    """
    Single-line input field.
    
    Example:
        >>> field = InputField(prompt="Enter name: ", max_length=30)
        >>> name = field.get_input()
    """
    
    def __init__(self,
                 prompt: str = "",
                 max_length: int = 255,
                 default: str = "",
                 mask: bool = False):
        """
        Initialize input field.
        
        Args:
            prompt: Field prompt
            max_length: Maximum input length
            default: Default value
            mask: Mask input (password field)
        """
        self.prompt = prompt
        self.max_length = max_length
        self.default = default
        self.mask = mask
        
    def get_input(self) -> Optional[str]:
        """
        Display field and get input.
        
        Returns:
            Input string or None on ESC
        """
        write(self.prompt)
        
        buffer = list(self.default)
        cursor_pos = len(buffer)
        
        # Display default
        if buffer:
            if self.mask:
                write("*" * len(buffer))
            else:
                write(''.join(buffer))
        
        with RawInput() as inp:
            while True:
                key = inp.get_key()
                
                if key == KEY_ENTER:
                    writeln()
                    return ''.join(buffer)
                    
                elif key == KEY_ESC:
                    writeln()
                    return None
                    
                elif key == KEY_BACKSPACE or key == KEY_DELETE:
                    if cursor_pos > 0:
                        buffer.pop(cursor_pos - 1)
                        cursor_pos -= 1
                        # Redraw
                        write('\r' + self.prompt)
                        if self.mask:
                            write("*" * len(buffer) + " ")
                        else:
                            write(''.join(buffer) + " ")
                        clear_to_eol()
                        # Position cursor
                        write('\r' + self.prompt)
                        if self.mask:
                            write("*" * cursor_pos)
                        else:
                            write(''.join(buffer[:cursor_pos]))
                            
                elif key == KEY_LEFT:
                    if cursor_pos > 0:
                        cursor_pos -= 1
                        write("\x1b[D")
                        
                elif key == KEY_RIGHT:
                    if cursor_pos < len(buffer):
                        cursor_pos += 1
                        write("\x1b[C")
                        
                elif key == KEY_HOME:
                    cursor_pos = 0
                    write('\r' + self.prompt)
                    
                elif key == KEY_END:
                    cursor_pos = len(buffer)
                    write('\r' + self.prompt)
                    if self.mask:
                        write("*" * len(buffer))
                    else:
                        write(''.join(buffer))
                        
                elif key and len(key) == 1 and is_printable(key):
                    if len(buffer) < self.max_length:
                        buffer.insert(cursor_pos, key)
                        cursor_pos += 1
                        # Redraw from cursor
                        if self.mask:
                            write("*" * (len(buffer) - cursor_pos + 1))
                            if cursor_pos < len(buffer):
                                write(f"\x1b[{len(buffer) - cursor_pos}D")
                        else:
                            write(''.join(buffer[cursor_pos-1:]))
                            if cursor_pos < len(buffer):
                                write(f"\x1b[{len(buffer) - cursor_pos}D")


class MultiLineEditor:
    """
    Multi-line text editor.
    
    Example:
        >>> editor = MultiLineEditor(max_lines=20, max_width=80)
        >>> text = editor.edit(initial="Hello\\nWorld")
    """
    
    def __init__(self,
                 max_lines: int = 100,
                 max_width: int = 80,
                 x: int = 1,
                 y: int = 1):
        """
        Initialize multi-line editor.
        
        Args:
            max_lines: Maximum number of lines
            max_width: Maximum line width
            x: Editor X position (1-indexed)
            y: Editor Y position (1-indexed)
        """
        self.max_lines = max_lines
        self.max_width = max_width
        self.x = x
        self.y = y
        
    def edit(self, initial: str = "") -> Optional[str]:
        """
        Edit text.
        
        Args:
            initial: Initial text
            
        Returns:
            Edited text or None on ESC
        """
        lines = initial.split('\n') if initial else ['']
        cursor_line = 0
        cursor_col = 0
        
        def redraw():
            for i, line in enumerate(lines):
                goto_xy(self.x, self.y + i)
                write(line)
                clear_to_eol()
            # Clear any extra lines
            for i in range(len(lines), self.max_lines):
                goto_xy(self.x, self.y + i)
                clear_to_eol()
                
        redraw()
        goto_xy(self.x + cursor_col, self.y + cursor_line)
        
        with RawInput() as inp:
            while True:
                key = inp.get_key()
                
                if key == KEY_ESC:
                    return None
                    
                elif key == KEY_ENTER:
                    # Insert new line
                    if len(lines) < self.max_lines:
                        current_line = lines[cursor_line]
                        lines[cursor_line] = current_line[:cursor_col]
                        lines.insert(cursor_line + 1, current_line[cursor_col:])
                        cursor_line += 1
                        cursor_col = 0
                        redraw()
                        goto_xy(self.x + cursor_col, self.y + cursor_line)
                        
                elif key == KEY_BACKSPACE or key == KEY_DELETE:
                    if cursor_col > 0:
                        # Delete character
                        line = lines[cursor_line]
                        lines[cursor_line] = line[:cursor_col-1] + line[cursor_col:]
                        cursor_col -= 1
                        goto_xy(self.x, self.y + cursor_line)
                        write(lines[cursor_line])
                        clear_to_eol()
                        goto_xy(self.x + cursor_col, self.y + cursor_line)
                    elif cursor_line > 0:
                        # Join with previous line
                        prev_line = lines[cursor_line - 1]
                        current_line = lines[cursor_line]
                        lines[cursor_line - 1] = prev_line + current_line
                        lines.pop(cursor_line)
                        cursor_line -= 1
                        cursor_col = len(prev_line)
                        redraw()
                        goto_xy(self.x + cursor_col, self.y + cursor_line)
                        
                elif key == KEY_UP:
                    if cursor_line > 0:
                        cursor_line -= 1
                        cursor_col = min(cursor_col, len(lines[cursor_line]))
                        goto_xy(self.x + cursor_col, self.y + cursor_line)
                        
                elif key == KEY_DOWN:
                    if cursor_line < len(lines) - 1:
                        cursor_line += 1
                        cursor_col = min(cursor_col, len(lines[cursor_line]))
                        goto_xy(self.x + cursor_col, self.y + cursor_line)
                        
                elif key == KEY_LEFT:
                    if cursor_col > 0:
                        cursor_col -= 1
                        goto_xy(self.x + cursor_col, self.y + cursor_line)
                    elif cursor_line > 0:
                        cursor_line -= 1
                        cursor_col = len(lines[cursor_line])
                        goto_xy(self.x + cursor_col, self.y + cursor_line)
                        
                elif key == KEY_RIGHT:
                    if cursor_col < len(lines[cursor_line]):
                        cursor_col += 1
                        goto_xy(self.x + cursor_col, self.y + cursor_line)
                    elif cursor_line < len(lines) - 1:
                        cursor_line += 1
                        cursor_col = 0
                        goto_xy(self.x + cursor_col, self.y + cursor_line)
                        
                elif key == KEY_HOME:
                    cursor_col = 0
                    goto_xy(self.x + cursor_col, self.y + cursor_line)
                    
                elif key == KEY_END:
                    cursor_col = len(lines[cursor_line])
                    goto_xy(self.x + cursor_col, self.y + cursor_line)
                    
                elif key and len(key) == 1 and is_printable(key):
                    # Insert character
                    line = lines[cursor_line]
                    if len(line) < self.max_width:
                        lines[cursor_line] = line[:cursor_col] + key + line[cursor_col:]
                        cursor_col += 1
                        goto_xy(self.x, self.y + cursor_line)
                        write(lines[cursor_line])
                        goto_xy(self.x + cursor_col, self.y + cursor_line)
                        
                # Check for save command (Ctrl+S would be '\x13')
                elif key == '\x13':  # Ctrl+S
                    return '\n'.join(lines)


@dataclass
class FormField:
    name: str
    x: int
    y: int
    width: int
    label: str = ""
    value: str = ""
    max_length: int = 255
    mask: bool = False
    hotkey: str = ""
    required: bool = False
    format_mask: str = ""
    options: List[Tuple[str, str]] = field(default_factory=list)
    label_color: str = ""
    input_color: str = ""
    focus_color: str = ""
    normalize: Optional[Callable[[str], str]] = None
    validate: Optional[Callable[[str], bool]] = None


class AdvancedForm:
    def __init__(
        self,
        fields: List[FormField],
        wrap: bool = True,
        save_mode: str = "ctrl_s",
        label_color: Optional[str] = None,
        input_color: Optional[str] = None,
        focus_color: Optional[str] = None,
        required_splash_text: str = "",
        required_splash_x: int = 1,
        required_splash_y: int = 24,
        required_splash_color: Optional[str] = None,
        theme: Optional[Theme] = None,
    ):
        th = get_current_theme() if theme is None else theme
        label_color = resolve_style(label_color, th.colors.label)
        input_color = resolve_style(input_color, th.colors.input)
        focus_color = resolve_style(focus_color, th.colors.input_focused)
        required_splash_color = resolve_style(required_splash_color, th.colors.warning)

        self.fields = fields
        self.wrap = wrap
        self.save_mode = save_mode
        self.selected_index = 0
        self._default_label_color = label_color
        self._default_input_color = input_color
        self._default_focus_color = focus_color
        self.required_splash_text = required_splash_text
        self.required_splash_x = required_splash_x
        self.required_splash_y = required_splash_y
        self.required_splash_color = required_splash_color
        self._required_splash_active = False
        self._required_splash_last_len = 0

    def _show_required_splash(self) -> None:
        if not self.required_splash_text:
            return
        self._required_splash_active = True
        msg_len = visible_length(self.required_splash_text)
        self._required_splash_last_len = max(self._required_splash_last_len, msg_len)
        goto_xy(self.required_splash_x, self.required_splash_y)
        write(
            f"{self.required_splash_color}{self.required_splash_text}{' ' * max(0, self._required_splash_last_len - msg_len)}{RESET}"
        )

    def _clear_required_splash(self) -> None:
        if not self._required_splash_active:
            return
        goto_xy(self.required_splash_x, self.required_splash_y)
        if self._required_splash_last_len > 0:
            write(f"{self.required_splash_color}{' ' * self._required_splash_last_len}{RESET}")
        else:
            clear_to_eol()
        self._required_splash_active = False

    def _field_center_x(self, index: int) -> float:
        f = self.fields[index]
        return float(f.x) + (float(f.width) - 1.0) / 2.0

    def _field_label_x(self, f: FormField) -> int:
        if not f.label:
            return f.x
        return max(1, f.x - (len(f.label) + 2))

    def _field_label_text(self, f: FormField) -> str:
        if not f.label:
            return ""
        return f"{f.label}: "

    def _mask_is_placeholder(self, ch: str) -> bool:
        return ch in ("0", "A", "X")

    def _mask_placeholder_display(self, ch: str) -> str:
        if ch == "0":
            return "0"
        return "_"

    def _mask_placeholder_ok(self, placeholder: str, ch: str) -> bool:
        if placeholder == "0":
            return ch.isdigit()
        if placeholder == "A":
            return ch.isalpha()
        if placeholder == "X":
            return ch.isalnum()
        return False

    def _mask_positions(self, mask: str) -> List[int]:
        return [i for i, ch in enumerate(mask) if self._mask_is_placeholder(ch)]

    def _mask_apply(self, raw_value: str, mask: str) -> str:
        raw = list(raw_value or "")
        out: List[str] = []
        raw_i = 0
        for m in mask:
            if self._mask_is_placeholder(m):
                if raw_i < len(raw):
                    out.append(raw[raw_i])
                    raw_i += 1
                else:
                    out.append(self._mask_placeholder_display(m))
            else:
                out.append(m)
        return "".join(out)

    def _required_ok(self, f: FormField) -> bool:
        if not f.required:
            return True
        val = f.value or ""
        if f.format_mask:
            needed = len(self._mask_positions(f.format_mask))
            return len(val) >= needed
        return bool(val.strip())

    def _first_invalid_required_index(self) -> Optional[int]:
        for i, f in enumerate(self.fields):
            if not self._required_ok(f):
                return i
        return None

    def _colors_for_field(self, f: FormField, focused: bool) -> tuple[str, str]:
        label_color = f.label_color or self._default_label_color
        if focused:
            input_color = f.focus_color or self._default_focus_color or f.input_color or self._default_input_color
        else:
            input_color = f.input_color or self._default_input_color
        return label_color, input_color

    def draw_field(self, index: int, focused: bool) -> None:
        if index < 0 or index >= len(self.fields):
            return

        f = self.fields[index]
        label_color, input_color = self._colors_for_field(f, focused)

        if f.label:
            goto_xy(self._field_label_x(f), f.y)
            write(f"{label_color}{self._field_label_text(f)}{RESET}")

        if f.options:
            shown = ""
            if f.value:
                for ov, ol in f.options:
                    if ov == f.value:
                        shown = ol
                        break
            if not shown and f.options:
                shown = f.options[0][1]
        elif f.mask:
            shown = f.value
            if shown:
                shown = "*" * len(shown)
        elif f.format_mask:
            shown = self._mask_apply(f.value, f.format_mask)
        else:
            shown = f.value

        shown = shown[: max(0, f.width)]
        padded = pad_string(shown, f.width, "left")
        goto_xy(f.x, f.y)
        write(f"{input_color}{padded}{RESET}")

    def redraw(self) -> None:
        for i in range(len(self.fields)):
            self.draw_field(i, i == self.selected_index)

    def _find_neighbor(self, direction: str) -> Optional[int]:
        if not self.fields:
            return None

        cur_idx = self.selected_index
        cur = self.fields[cur_idx]
        cur_cx = self._field_center_x(cur_idx)
        cur_y = cur.y

        candidates = []
        for i, f in enumerate(self.fields):
            if i == cur_idx:
                continue
            cx = self._field_center_x(i)
            dy = f.y - cur_y
            dx = cx - cur_cx

            if direction == "down" and dy > 0:
                candidates.append((dy, abs(dx), i))
            elif direction == "up" and dy < 0:
                candidates.append((-dy, abs(dx), i))
            elif direction == "right" and dx > 0:
                candidates.append((abs(dy), dx, i))
            elif direction == "left" and dx < 0:
                candidates.append((abs(dy), -dx, i))

        if candidates:
            candidates.sort(key=lambda t: (t[0], t[1], t[2]))
            return candidates[0][2]

        if not self.wrap:
            return None

        if direction in ("up", "down"):
            if direction == "down":
                extreme_y = min(f.y for i, f in enumerate(self.fields) if i != cur_idx)
            else:
                extreme_y = max(f.y for i, f in enumerate(self.fields) if i != cur_idx)
            pool = [i for i, f in enumerate(self.fields) if i != cur_idx and f.y == extreme_y]
            pool.sort(key=lambda i: (abs(self._field_center_x(i) - cur_cx), abs(self.fields[i].y - cur_y), i))
            return pool[0] if pool else None

        if direction == "right":
            extreme = min(self._field_center_x(i) for i in range(len(self.fields)) if i != cur_idx)
            pool = [i for i in range(len(self.fields)) if i != cur_idx and self._field_center_x(i) == extreme]
        else:
            extreme = max(self._field_center_x(i) for i in range(len(self.fields)) if i != cur_idx)
            pool = [i for i in range(len(self.fields)) if i != cur_idx and self._field_center_x(i) == extreme]

        if not pool:
            return None
        pool.sort(key=lambda i: (abs(self.fields[i].y - cur_y), abs(self._field_center_x(i) - cur_cx), i))
        return pool[0]

    def _edit_field(self, index: int, inp: RawInput) -> bool:
        f = self.fields[index]
        label_color, input_color = self._colors_for_field(f, True)

        if f.options:
            original = f.value

            opt_i = 0
            if f.value:
                for i, (ov, _ol) in enumerate(f.options):
                    if ov == f.value:
                        opt_i = i
                        break

            def redraw_line_opt() -> None:
                shown = f.options[opt_i][1] if f.options else ""
                shown = shown[: max(0, f.width)]
                padded = pad_string(shown, f.width, "left")
                goto_xy(f.x, f.y)
                write(f"{input_color}{padded}{RESET}")

            if f.label:
                goto_xy(self._field_label_x(f), f.y)
                write(f"{label_color}{self._field_label_text(f)}{RESET}")

            hide_cursor()
            redraw_line_opt()

            while True:
                key = inp.get_key()

                if key == KEY_ENTER:
                    if f.options:
                        f.value = f.options[opt_i][0]
                        if f.normalize:
                            f.value = f.normalize(f.value)
                    hide_cursor()
                    self.draw_field(index, True)
                    return True

                if key == KEY_ESC:
                    f.value = original
                    hide_cursor()
                    self.draw_field(index, True)
                    return False

                if key in (KEY_LEFT, KEY_UP):
                    if f.options:
                        opt_i = (opt_i - 1) % len(f.options)
                        redraw_line_opt()
                    continue

                if key in (KEY_RIGHT, KEY_DOWN, ' '):
                    if f.options:
                        opt_i = (opt_i + 1) % len(f.options)
                        redraw_line_opt()
                    continue

                if key and len(key) == 1 and is_printable(key):
                    ch = key.lower()
                    for i, (_ov, ol) in enumerate(f.options):
                        if ol and ol[0].lower() == ch:
                            opt_i = i
                            redraw_line_opt()
                            break
                    continue

        if f.format_mask:
            mask = f.format_mask
            positions = self._mask_positions(mask)
            effective_max = min(f.max_length, len(positions))
            raw = list((f.value or "")[:effective_max])

            cursor_i = min(len(raw), len(positions))

            def redraw_line_mask() -> None:
                shown = self._mask_apply(''.join(raw), mask)
                padded = pad_string(shown[:f.width], f.width, "left")
                goto_xy(f.x, f.y)
                write(f"{input_color}{padded}{RESET}")
                if positions:
                    if cursor_i >= len(positions):
                        goto_xy(f.x + positions[-1] + 1, f.y)
                    else:
                        goto_xy(f.x + positions[cursor_i], f.y)
                else:
                    goto_xy(f.x, f.y)

            if f.label:
                goto_xy(self._field_label_x(f), f.y)
                write(f"{label_color}{self._field_label_text(f)}{RESET}")

            show_cursor()
            redraw_line_mask()

            while True:
                key = inp.get_key()
                if key == KEY_ENTER:
                    new_val = ''.join(raw)
                    if f.normalize:
                        new_val = f.normalize(new_val)
                    if f.required and len(new_val) < min(len(positions), effective_max):
                        redraw_line_mask()
                        continue
                    if f.validate and not f.validate(new_val):
                        redraw_line_mask()
                        continue
                    f.value = new_val
                    hide_cursor()
                    self.draw_field(index, True)
                    return True

                if key == KEY_ESC:
                    hide_cursor()
                    self.draw_field(index, True)
                    return False

                if key == KEY_BACKSPACE or key == KEY_DELETE:
                    if cursor_i > 0 and raw:
                        if cursor_i > len(raw):
                            cursor_i = len(raw)
                        cursor_i -= 1
                        if cursor_i < len(raw):
                            raw.pop(cursor_i)
                        redraw_line_mask()
                    continue

                if key == KEY_LEFT:
                    if cursor_i > 0:
                        cursor_i -= 1
                        redraw_line_mask()
                    continue

                if key == KEY_RIGHT:
                    if cursor_i < len(positions):
                        cursor_i += 1
                        redraw_line_mask()
                    continue

                if key == KEY_HOME:
                    cursor_i = 0
                    redraw_line_mask()
                    continue

                if key == KEY_END:
                    cursor_i = len(positions)
                    redraw_line_mask()
                    continue

                if key and len(key) == 1 and is_printable(key):
                    ch = key
                    if cursor_i < len(positions):
                        placeholder = mask[positions[cursor_i]]
                        if self._mask_placeholder_ok(placeholder, ch):
                            if cursor_i < len(raw):
                                raw[cursor_i] = ch
                            elif len(raw) < effective_max:
                                raw.append(ch)
                            else:
                                continue
                            cursor_i = min(cursor_i + 1, len(positions))
                            redraw_line_mask()
                    continue

            

        effective_max = min(f.max_length, f.width)
        buf = list((f.value or "")[:effective_max])
        cursor_pos = len(buf)

        def redraw_line() -> None:
            shown = ''.join(buf)
            if f.mask and shown:
                shown = "*" * len(shown)
            padded = pad_string(shown[:f.width], f.width, "left")
            goto_xy(f.x, f.y)
            write(f"{input_color}{padded}{RESET}")
            goto_xy(f.x + cursor_pos, f.y)

        if f.label:
            goto_xy(self._field_label_x(f), f.y)
            write(f"{label_color}{self._field_label_text(f)}{RESET}")

        show_cursor()
        redraw_line()

        while True:
            key = inp.get_key()
            if key == KEY_ENTER:
                new_val = ''.join(buf)
                if f.normalize:
                    new_val = f.normalize(new_val)
                if f.required and not new_val.strip():
                    redraw_line()
                    continue
                if f.validate and not f.validate(new_val):
                    redraw_line()
                    continue
                f.value = new_val
                hide_cursor()
                self.draw_field(index, True)
                return True

            if key == KEY_ESC:
                hide_cursor()
                self.draw_field(index, True)
                return False

            if key == KEY_BACKSPACE or key == KEY_DELETE:
                if cursor_pos > 0:
                    buf.pop(cursor_pos - 1)
                    cursor_pos -= 1
                    redraw_line()
                continue

            if key == KEY_LEFT:
                if cursor_pos > 0:
                    cursor_pos -= 1
                    goto_xy(f.x + cursor_pos, f.y)
                continue

            if key == KEY_RIGHT:
                if cursor_pos < len(buf):
                    cursor_pos += 1
                    goto_xy(f.x + cursor_pos, f.y)
                continue

            if key == KEY_HOME:
                cursor_pos = 0
                goto_xy(f.x, f.y)
                continue

            if key == KEY_END:
                cursor_pos = len(buf)
                goto_xy(f.x + cursor_pos, f.y)
                continue

            if key and len(key) == 1 and is_printable(key):
                if len(buf) < effective_max:
                    buf.insert(cursor_pos, key)
                    cursor_pos += 1
                    redraw_line()
                continue

    def _show_esc_prompt(self) -> Optional[str]:
        """Show ESC prompt and return action: 'edit', 'save', or 'exit'."""
        from .ansi import YELLOW, GREEN, RED, BOLD
        from .input import RawInput
        goto_xy(1, 24)
        write(f"{YELLOW}{BOLD}Keep (E)diting, (S)ave the form, or E(X)it?{RESET} ")
        show_cursor()
        
        with RawInput() as inp:
            while True:
                ch = inp.get_key()
                if not ch:
                    continue
                ch_lower = ch.lower()
                if ch_lower == 'e':
                    hide_cursor()
                    goto_xy(1, 24)
                    clear_to_eol()
                    return 'edit'
                elif ch_lower == 's':
                    hide_cursor()
                    goto_xy(1, 24)
                    clear_to_eol()
                    return 'save'
                elif ch_lower == 'x':
                    hide_cursor()
                    goto_xy(1, 24)
                    clear_to_eol()
                    return 'exit'
    
    def run(self) -> Optional[Dict[str, str]]:
        hide_cursor()
        self.redraw()

        with RawInput() as inp:
            while True:
                key = inp.get_key()

                if self._required_splash_active:
                    self._clear_required_splash()
                
                if key == KEY_ESC:
                    if self.save_mode == "esc_prompt":
                        action = self._show_esc_prompt()
                        if action == 'edit':
                            self.redraw()
                            continue
                        elif action == 'save':
                            invalid = self._first_invalid_required_index()
                            if invalid is not None:
                                self._show_required_splash()
                                old_index = self.selected_index
                                self.selected_index = invalid
                                if old_index != self.selected_index:
                                    self.draw_field(old_index, False)
                                    self.draw_field(self.selected_index, True)
                                continue
                            show_cursor()
                            return {f.name: f.value for f in self.fields}
                        else:  # exit
                            show_cursor()
                            return None
                    else:
                        show_cursor()
                        return None

                if key == '\x13':  # Ctrl+S
                    if self.save_mode in ("ctrl_s", "esc_prompt"):
                        invalid = self._first_invalid_required_index()
                        if invalid is not None:
                            self._show_required_splash()
                            old_index = self.selected_index
                            self.selected_index = invalid
                            if old_index != self.selected_index:
                                self.draw_field(old_index, False)
                                self.draw_field(self.selected_index, True)
                            continue
                        show_cursor()
                        return {f.name: f.value for f in self.fields}

                # Check for field hotkeys
                if key and len(key) == 1 and is_printable(key):
                    key_lower = key.lower()
                    for i, f in enumerate(self.fields):
                        if f.hotkey and f.hotkey.lower() == key_lower:
                            old_index = self.selected_index
                            self.selected_index = i
                            if old_index != self.selected_index:
                                self.draw_field(old_index, False)
                                self.draw_field(self.selected_index, True)
                            continue
                
                old_index = self.selected_index

                if key == KEY_UP:
                    nxt = self._find_neighbor("up")
                    if nxt is not None:
                        self.selected_index = nxt
                elif key == KEY_DOWN:
                    nxt = self._find_neighbor("down")
                    if nxt is not None:
                        self.selected_index = nxt
                elif key == KEY_LEFT:
                    nxt = self._find_neighbor("left")
                    if nxt is not None:
                        self.selected_index = nxt
                elif key == KEY_RIGHT:
                    nxt = self._find_neighbor("right")
                    if nxt is not None:
                        self.selected_index = nxt
                elif key == KEY_ENTER:
                    self._edit_field(self.selected_index, inp)
                
                if old_index != self.selected_index:
                    self.draw_field(old_index, False)
                    self.draw_field(self.selected_index, True)
