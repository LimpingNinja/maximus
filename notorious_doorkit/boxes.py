"""
Box drawing helpers.
"""

from dataclasses import dataclass
from typing import Dict

from .ansi import goto_xy, write, RESET


@dataclass(frozen=True)
class BoxStyle:
    tl: str
    tr: str
    bl: str
    br: str
    h: str
    v: str


_BOX_STYLES: Dict[str, BoxStyle] = {
    "ascii": BoxStyle(tl="+", tr="+", bl="+", br="+", h="-", v="|"),
    "single": BoxStyle(tl="┌", tr="┐", bl="└", br="┘", h="─", v="│"),
    "double": BoxStyle(tl="╔", tr="╗", bl="╚", br="╝", h="═", v="║"),
}


def ascii_box(width: int, height: int, style: str = "double") -> str:
    """Return a box as a CRLF-delimited string."""
    if width < 2 or height < 2:
        return ""

    st = _BOX_STYLES.get(style, _BOX_STYLES["double"])
    top = st.tl + (st.h * (width - 2)) + st.tr
    mid = st.v + (" " * (width - 2)) + st.v
    bot = st.bl + (st.h * (width - 2)) + st.br

    lines = [top]
    for _ in range(height - 2):
        lines.append(mid)
    lines.append(bot)

    return "\r\n".join(lines)


def ansi_box(
    x: int,
    y: int,
    width: int,
    height: int,
    style: str = "double",
    border_color: str = "",
    fill_color: str = "",
    fill_char: str = " ",
) -> None:
    """Draw a box at screen position (1-indexed), optionally filled and colored."""
    if width < 2 or height < 2:
        return

    st = _BOX_STYLES.get(style, _BOX_STYLES["double"])

    top = st.tl + (st.h * (width - 2)) + st.tr
    bot = st.bl + (st.h * (width - 2)) + st.br

    # Top
    goto_xy(x, y)
    write(f"{border_color}{top}{RESET}")

    # Middle rows
    inner = (fill_char * (width - 2))
    for yy in range(y + 1, y + height - 1):
        goto_xy(x, yy)
        write(f"{border_color}{st.v}{RESET}")
        if width > 2:
            goto_xy(x + 1, yy)
            write(f"{fill_color}{inner}{RESET}")
        goto_xy(x + width - 1, yy)
        write(f"{border_color}{st.v}{RESET}")

    # Bottom
    goto_xy(x, y + height - 1)
    write(f"{border_color}{bot}{RESET}")
