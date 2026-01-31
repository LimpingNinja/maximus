from __future__ import annotations

from dataclasses import dataclass, field
from typing import List, Optional, Tuple

from .ansi import set_cursor, write
from .text import get_attrib, sgr_from_pc_attr


DEFAULT_WIDTH = 80
DEFAULT_HEIGHT = 25


SCROLL_NORMAL = 0x0000
SCROLL_NO_CLEAR = 0x0001


@dataclass(frozen=True)
class Cell:
    ch: str = " "
    attr: int = 0x07


@dataclass(frozen=True)
class ScreenBlock:
    width: int
    height: int
    cells: List[Cell]

    def get(self, row: int, col: int) -> Cell:
        r = int(row)
        c = int(col)
        if r < 1 or c < 1 or r > self.height or c > self.width:
            return Cell()
        return self.cells[(r - 1) * self.width + (c - 1)]


@dataclass(frozen=True)
class ScreenSnapshot:
    width: int
    height: int
    cursor_row: int
    cursor_col: int
    current_attr: int
    cells: List[Cell]


@dataclass
class WindowHandle:
    left: int
    top: int
    right: int
    bottom: int
    under: ScreenBlock


@dataclass
class ShadowScreen:
    width: int = DEFAULT_WIDTH
    height: int = DEFAULT_HEIGHT
    cursor_row: int = 1
    cursor_col: int = 1
    current_attr: int = 0x07
    _cells: List[Cell] = field(default_factory=list)
    _windows: List[WindowHandle] = field(default_factory=list)

    def __post_init__(self) -> None:
        if not self._cells:
            self._cells = [Cell(" ", int(self.current_attr) & 0xFF) for _ in range(self.width * self.height)]

    def _idx(self, row: int, col: int) -> int:
        return (int(row) - 1) * self.width + (int(col) - 1)

    def get_cell(self, row: int, col: int) -> Cell:
        r = int(row)
        c = int(col)
        if r < 1 or c < 1 or r > self.height or c > self.width:
            return Cell()
        return self._cells[self._idx(r, c)]

    def set_cell(self, row: int, col: int, ch: str, attr: Optional[int] = None) -> None:
        r = int(row)
        c = int(col)
        if r < 1 or c < 1 or r > self.height or c > self.width:
            return
        s = "" if ch is None else str(ch)
        if not s:
            s = " "
        a = int(self.current_attr if attr is None else attr) & 0xFF
        self._cells[self._idx(r, c)] = Cell(s[0], a)

    def goto(self, row: int, col: int) -> None:
        r = int(row)
        c = int(col)
        if r < 1:
            r = 1
        if c < 1:
            c = 1
        if r > self.height:
            r = self.height
        if c > self.width:
            c = self.width
        self.cursor_row = r
        self.cursor_col = c

    def get_cursor(self) -> Tuple[int, int]:
        return (int(self.cursor_row), int(self.cursor_col))

    def set_attrib(self, attr: Optional[int] = None) -> int:
        a = int(get_attrib() if attr is None else attr) & 0xFF
        self.current_attr = a
        return a

    def write_text(self, text: str, *, attr: Optional[int] = None) -> None:
        a = int(self.current_attr if attr is None else attr) & 0xFF
        s = "" if text is None else str(text)
        for ch in s:
            if ch == "\r":
                self.cursor_col = 1
                continue
            if ch == "\n":
                self.cursor_row = min(self.height, self.cursor_row + 1)
                continue
            self.set_cell(self.cursor_row, self.cursor_col, ch, a)
            self.cursor_col += 1
            if self.cursor_col > self.width:
                self.cursor_col = 1
                self.cursor_row = min(self.height, self.cursor_row + 1)

    def clear(self, *, attr: Optional[int] = None) -> None:
        a = int(self.current_attr if attr is None else attr) & 0xFF
        self._cells = [Cell(" ", a) for _ in range(self.width * self.height)]
        self.cursor_row = 1
        self.cursor_col = 1
        self.current_attr = a

    def gettext(self, left: int, top: int, right: int, bottom: int) -> ScreenBlock:
        l = int(left)
        t = int(top)
        r = int(right)
        b = int(bottom)
        if l < 1:
            l = 1
        if t < 1:
            t = 1
        if r > self.width:
            r = self.width
        if b > self.height:
            b = self.height
        w = max(0, r - l + 1)
        h = max(0, b - t + 1)
        out: List[Cell] = []
        for rr in range(t, t + h):
            for cc in range(l, l + w):
                out.append(self.get_cell(rr, cc))
        return ScreenBlock(width=w, height=h, cells=out)

    def puttext(self, left: int, top: int, block: ScreenBlock) -> None:
        l = int(left)
        t = int(top)
        for rr in range(1, int(block.height) + 1):
            for cc in range(1, int(block.width) + 1):
                cell = block.get(rr, cc)
                self.set_cell(t + rr - 1, l + cc - 1, cell.ch, cell.attr)

    def save_screen(self) -> ScreenSnapshot:
        return ScreenSnapshot(
            width=int(self.width),
            height=int(self.height),
            cursor_row=int(self.cursor_row),
            cursor_col=int(self.cursor_col),
            current_attr=int(self.current_attr) & 0xFF,
            cells=list(self._cells),
        )

    def restore_screen(self, snap: ScreenSnapshot) -> None:
        self.width = int(snap.width)
        self.height = int(snap.height)
        self.cursor_row = int(snap.cursor_row)
        self.cursor_col = int(snap.cursor_col)
        self.current_attr = int(snap.current_attr) & 0xFF
        self._cells = list(snap.cells)
        self.paint_all()

    def paint_region(self, left: int, top: int, right: int, bottom: int) -> None:
        l = int(left)
        t = int(top)
        r = int(right)
        b = int(bottom)
        if l < 1:
            l = 1
        if t < 1:
            t = 1
        if r > self.width:
            r = self.width
        if b > self.height:
            b = self.height

        last_attr: Optional[int] = None
        for rr in range(t, b + 1):
            set_cursor(rr, l)
            parts: List[str] = []
            for cc in range(l, r + 1):
                cell = self.get_cell(rr, cc)
                a = int(cell.attr) & 0xFF
                if last_attr is None or a != last_attr:
                    parts.append(sgr_from_pc_attr(a))
                    last_attr = a
                parts.append(cell.ch)
            write("".join(parts))

        if last_attr is not None:
            self.current_attr = int(last_attr) & 0xFF

    def paint_all(self) -> None:
        self.paint_region(1, 1, self.width, self.height)
        set_cursor(self.cursor_row, self.cursor_col)

    def window_create(
        self,
        left: int,
        top: int,
        right: int,
        bottom: int,
        *,
        fill_attr: Optional[int] = None,
        border_attr: Optional[int] = None,
        title: Optional[str] = None,
    ) -> WindowHandle:
        l = int(left)
        t = int(top)
        r = int(right)
        b = int(bottom)
        under = self.gettext(l, t, r, b)

        fa = int(self.current_attr if fill_attr is None else fill_attr) & 0xFF
        ba = int(fa if border_attr is None else border_attr) & 0xFF

        for rr in range(t, b + 1):
            for cc in range(l, r + 1):
                self.set_cell(rr, cc, " ", fa)

        if r - l >= 1 and b - t >= 1:
            self.set_cell(t, l, "+", ba)
            self.set_cell(t, r, "+", ba)
            self.set_cell(b, l, "+", ba)
            self.set_cell(b, r, "+", ba)
            for cc in range(l + 1, r):
                self.set_cell(t, cc, "-", ba)
                self.set_cell(b, cc, "-", ba)
            for rr in range(t + 1, b):
                self.set_cell(rr, l, "|", ba)
                self.set_cell(rr, r, "|", ba)

            if title:
                s = str(title)
                max_len = max(0, (r - l + 1) - 4)
                if max_len > 0:
                    s = s[:max_len]
                    start = l + 2
                    for i, ch in enumerate(s):
                        self.set_cell(t, start + i, ch, ba)

        handle = WindowHandle(left=l, top=t, right=r, bottom=b, under=under)
        self._windows.append(handle)
        self.paint_region(l, t, r, b)
        return handle

    def window_remove(self, handle: WindowHandle) -> None:
        if handle in self._windows:
            while self._windows:
                h = self._windows.pop()
                self.puttext(h.left, h.top, h.under)
                self.paint_region(h.left, h.top, h.right, h.bottom)
                if h is handle:
                    break

    def scroll(self, left: int, top: int, right: int, bottom: int, distance: int, flags: int = SCROLL_NORMAL) -> None:
        l = int(left)
        t = int(top)
        r = int(right)
        b = int(bottom)
        d = int(distance)
        f = int(flags)

        if d == 0:
            return

        if l < 1:
            l = 1
        if t < 1:
            t = 1
        if r > self.width:
            r = self.width
        if b > self.height:
            b = self.height
        if l > r or t > b:
            return

        h = b - t + 1
        w = r - l + 1
        if h <= 0 or w <= 0:
            return

        if d > h:
            d = h
        if d < -h:
            d = -h

        orig_row = int(self.cursor_row)
        orig_col = int(self.cursor_col)
        orig_attr = int(self.current_attr) & 0xFF

        block = self.gettext(l, t, r, b)

        no_clear = (f & SCROLL_NO_CLEAR) != 0

        for rr in range(1, h + 1):
            for cc in range(1, w + 1):
                src_rr = rr + d
                if 1 <= src_rr <= h:
                    cell = block.get(src_rr, cc)
                    self.set_cell(t + rr - 1, l + cc - 1, cell.ch, cell.attr)
                else:
                    if not no_clear:
                        self.set_cell(t + rr - 1, l + cc - 1, " ", orig_attr)

        self.paint_region(l, t, r, b)
        self.current_attr = orig_attr
        set_cursor(orig_row, orig_col)
        write(sgr_from_pc_attr(orig_attr))


_DEFAULT_SCREEN: Optional[ShadowScreen] = None


def get_screen() -> ShadowScreen:
    global _DEFAULT_SCREEN
    if _DEFAULT_SCREEN is None:
        _DEFAULT_SCREEN = ShadowScreen(width=DEFAULT_WIDTH, height=DEFAULT_HEIGHT)
    return _DEFAULT_SCREEN


def set_screen(screen: ShadowScreen) -> None:
    global _DEFAULT_SCREEN
    _DEFAULT_SCREEN = screen


def get_cursor() -> Tuple[int, int]:
    return get_screen().get_cursor()


def save_screen() -> ScreenSnapshot:
    return get_screen().save_screen()


def restore_screen(snap: ScreenSnapshot) -> None:
    get_screen().restore_screen(snap)


def gettext(left: int, top: int, right: int, bottom: int) -> ScreenBlock:
    return get_screen().gettext(left, top, right, bottom)


def puttext(left: int, top: int, block: ScreenBlock) -> None:
    get_screen().puttext(left, top, block)


def window_create(
    left: int,
    top: int,
    right: int,
    bottom: int,
    *,
    fill_attr: Optional[int] = None,
    border_attr: Optional[int] = None,
    title: Optional[str] = None,
) -> WindowHandle:
    return get_screen().window_create(
        left,
        top,
        right,
        bottom,
        fill_attr=fill_attr,
        border_attr=border_attr,
        title=title,
    )


def window_remove(handle: WindowHandle) -> None:
    get_screen().window_remove(handle)


def scroll(left: int, top: int, right: int, bottom: int, distance: int, flags: int = SCROLL_NORMAL) -> None:
    get_screen().scroll(left, top, right, bottom, distance, flags)


def scroll_region(left: int, top: int, right: int, bottom: int, distance: int, flags: int = SCROLL_NORMAL) -> None:
    scroll(left, top, right, bottom, distance, flags)
