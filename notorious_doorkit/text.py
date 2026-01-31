from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, Dict, Optional, Union

from .ansi import BLINK, RESET
from .ansi import write
from .dropfiles import DropfileInfo
from .runtime import get_dropfile as _rt_dropfile
from .runtime import get_runtime_config as _rt_config
from .runtime import get_runtime as _get_runtime
from .runtime import get_terminal as _rt_terminal
from .utils import write_bytes


_DEFAULT_COLOR_NAMES = [
    "BLACK",
    "BLUE",
    "GREEN",
    "CYAN",
    "RED",
    "MAGENTA",
    "YELLOW",
    "WHITE",
    "BROWN",
    "GREY",
    "BRIGHT",
    "FLASHING",
]


@dataclass(frozen=True)
class Attr:
    value: int

    @classmethod
    def from_pc_attr(cls, attr: int) -> "Attr":
        return cls(int(attr) & 0xFF)

    @classmethod
    def from_color_desc(
        cls,
        color_desc: str,
        *,
        color_names: Optional[Dict[str, str]] = None,
    ) -> "Attr":
        return cls(pc_attr_from_color_desc(color_desc, color_names=color_names))

    def to_pc_attr(self) -> int:
        return int(self.value) & 0xFF

    def to_sgr(self) -> str:
        return sgr_from_pc_attr(self.to_pc_attr())

    def __int__(self) -> int:
        return self.to_pc_attr()


def sgr_from_pc_attr(attr: int) -> str:
    a = int(attr) & 0xFF
    fg = a & 0x07
    bright = (a & 0x08) != 0
    bg = (a >> 4) & 0x07
    flashing = (a & 0x80) != 0

    parts = [RESET]

    if flashing:
        parts.append(BLINK)

    fg_code = 30 + fg
    if bright:
        fg_code = 90 + fg

    bg_code = 40 + bg

    parts.append(f"\x1b[{fg_code}m")
    parts.append(f"\x1b[{bg_code}m")

    return "".join(parts)


_current_attr: int = 0x07


def get_attrib() -> int:
    return int(_current_attr) & 0xFF


def set_attrib(attr: Union[int, Attr]) -> int:
    global _current_attr

    a = int(attr) & 0xFF
    _current_attr = a
    write(sgr_from_pc_attr(a))
    return a


def pc_attr_from_color_desc(
    color_desc: str,
    *,
    color_names: Optional[Dict[str, str]] = None,
) -> int:
    names = {n.upper(): n for n in _DEFAULT_COLOR_NAMES}
    if color_names:
        for k, v in color_names.items():
            if not k:
                continue
            names[str(k).upper()] = str(v)

    tokens = [t for t in (color_desc or "").replace("\t", " ").split(" ") if t.strip()]

    attr = 0x07
    b_foreground = True

    for raw in tokens:
        tok = raw.strip().upper()
        if not tok:
            continue

        if tok in ("ON", "IN", "AT"):
            continue

        if tok not in names:
            continue

        if tok in ("BRIGHT",):
            attr |= 0x08
            continue

        if tok in ("FLASHING", "BLINK", "BLINKING"):
            attr |= 0x80
            continue

        color_id = _DEFAULT_COLOR_NAMES.index(names[tok].upper())

        if color_id <= 9:
            if color_id >= 8:
                color_id -= 2

            if b_foreground:
                b_foreground = False
                attr &= ~0x07
                attr |= (color_id & 0x07)
            else:
                attr &= ~0x70
                attr |= ((color_id & 0x07) << 4)

    return int(attr) & 0xFF


def sgr_from_color_desc(
    color_desc: str,
    *,
    color_names: Optional[Dict[str, str]] = None,
) -> str:
    return sgr_from_pc_attr(pc_attr_from_color_desc(color_desc, color_names=color_names))


@dataclass
class RaqbbsContext:
    username: Optional[str] = None
    location: Optional[str] = None
    home_phone: Optional[str] = None
    data_phone: Optional[str] = None


def ra_qbbs_context_from_dropfile(dropfile: Optional[DropfileInfo]) -> RaqbbsContext:
    if dropfile is None:
        return RaqbbsContext()
    u = dropfile.user
    return RaqbbsContext(
        username=(u.alias or u.full_name or "") or None,
        location=(u.city or "") or None,
        home_phone=(u.phone or "") or None,
        data_phone=None,
    )


def expand_ra_qbbs_control_codes(
    text: str,
    *,
    ctx: Optional[RaqbbsContext] = None,
    f_map: Optional[Dict[str, str]] = None,
    k_map: Optional[Dict[str, str]] = None,
) -> str:
    if not text:
        return ""

    ctx = ctx or RaqbbsContext()

    fm: Dict[str, str] = {
        "A": ctx.username or "",
        "B": ctx.location or "",
        "D": ctx.data_phone or "",
        "E": ctx.home_phone or "",
    }
    if f_map:
        fm.update({str(k).upper(): str(v) for k, v in f_map.items()})

    km: Dict[str, str] = {}
    if k_map:
        km.update({str(k).upper(): str(v) for k, v in k_map.items()})

    out: list[str] = []
    i = 0
    while i < len(text):
        ch = text[i]

        if ch == "\x06" and i + 1 < len(text):
            code = text[i + 1].upper()
            out.append(fm.get(code, ""))
            i += 2
            continue

        if ch == "\x0b" and i + 1 < len(text):
            code = text[i + 1].upper()
            out.append(km.get(code, ""))
            i += 2
            continue

        out.append(ch)
        i += 1

    return "".join(out)


def render_opendoors_text(
    text: str,
    *,
    delimiter: str = "`",
    color_char: Optional[str] = None,
    color_names: Optional[Dict[str, str]] = None,
    expand_ra_qbbs: bool = False,
    ra_qbbs_ctx: Optional[RaqbbsContext] = None,
    ra_qbbs_f_map: Optional[Dict[str, str]] = None,
    ra_qbbs_k_map: Optional[Dict[str, str]] = None,
    unknown_color_handler: Optional[Callable[[str], str]] = None,
) -> str:
    if text is None:
        text = ""

    s = str(text)

    if expand_ra_qbbs:
        s = expand_ra_qbbs_control_codes(s, ctx=ra_qbbs_ctx, f_map=ra_qbbs_f_map, k_map=ra_qbbs_k_map)

    delim = str(delimiter or "")
    if delim and len(delim) != 1:
        raise ValueError("delimiter must be a single character")

    cchar = None
    if color_char:
        cchar = str(color_char)
        if len(cchar) != 1:
            raise ValueError("color_char must be a single character")

    if not delim and not cchar:
        return s

    out: list[str] = []
    i = 0
    while i < len(s):
        ch = s[i]

        if cchar and ch == cchar:
            if i + 1 >= len(s):
                return "".join(out)
            attr = ord(s[i + 1])
            out.append(sgr_from_pc_attr(attr))
            i += 2
            continue

        if delim and ch == delim:
            end = s.find(delim, i + 1)
            if end == -1:
                return "".join(out)

            desc = s[i + 1 : end]
            try:
                out.append(sgr_from_color_desc(desc, color_names=color_names))
            except Exception:
                if unknown_color_handler is not None:
                    out.append(unknown_color_handler(desc))

            i = end + 1
            continue

        out.append(ch)
        i += 1

    if i < len(s):
        out.append(s[i:])

    return "".join(out)


render = render_opendoors_text


def printf(
    fmt: str,
    *args: object,
    delimiter: Optional[str] = None,
    color_char: Optional[str] = None,
    color_names: Optional[Dict[str, str]] = None,
    expand_ra_qbbs: Optional[bool] = None,
    ra_qbbs_ctx: Optional[RaqbbsContext] = None,
    ra_qbbs_f_map: Optional[Dict[str, str]] = None,
    ra_qbbs_k_map: Optional[Dict[str, str]] = None,
    unknown_color_handler: Optional[Callable[[str], str]] = None,
    **kwargs: object,
) -> None:
    if args and kwargs:
        raise TypeError("printf() does not accept both *args and **kwargs")

    # Prefer runtime defaults for session-aware features.
    _get_runtime()

    s = "" if fmt is None else str(fmt)

    if args:
        s = s % args
    elif kwargs:
        s = s % kwargs
    else:
        try:
            s = s % ()
        except TypeError:
            pass

    cfg = _rt_config()
    if delimiter is None:
        delimiter = cfg.text.color_delimiter
    if color_char is None:
        color_char = cfg.text.color_char
    if expand_ra_qbbs is None:
        expand_ra_qbbs = cfg.text.expand_ra_qbbs

    if expand_ra_qbbs and ra_qbbs_ctx is None:
        ra_qbbs_ctx = ra_qbbs_context_from_dropfile(_rt_dropfile())

    out = render_opendoors_text(
        s,
        delimiter=delimiter,
        color_char=color_char,
        color_names=color_names,
        expand_ra_qbbs=expand_ra_qbbs,
        ra_qbbs_ctx=ra_qbbs_ctx,
        ra_qbbs_f_map=ra_qbbs_f_map,
        ra_qbbs_k_map=ra_qbbs_k_map,
        unknown_color_handler=unknown_color_handler,
    )

    write(out)


def sprintf(fmt: str, *args: object, **kwargs: object) -> str:
    if args and kwargs:
        raise TypeError("sprintf() does not accept both *args and **kwargs")

    s = "" if fmt is None else str(fmt)

    if args:
        s = s % args
    elif kwargs:
        s = s % kwargs
    else:
        try:
            s = s % ()
        except TypeError:
            pass

    return s


def disp_str(text: str) -> None:
    write("" if text is None else str(text))


def putch(ch: str) -> None:
    if ch is None:
        return
    s = str(ch)
    if not s:
        return
    write(s[0])


def repeat(ch: str, times: int) -> None:
    if ch is None:
        return
    n = int(times)
    if n <= 0:
        return
    s = str(ch)
    if not s:
        return
    write(s[0] * n)


def disp(buf: Union[str, bytes, bytearray], n: Optional[int] = None, local_echo: bool = False) -> None:
    if buf is None:
        return

    if isinstance(buf, (bytes, bytearray)):
        b = bytes(buf)
        if n is not None:
            b = b[: int(n)]
        write_bytes(b)
        return

    s = str(buf)
    if n is not None:
        s = s[: int(n)]
    write(s)


def _avatar_to_ansi(text: str) -> str:
    if not text:
        return ""

    out: list[str] = []
    i = 0
    while i < len(text):
        ch = text[i]
        code = ord(ch)

        if code == 0x19:
            if i + 2 >= len(text):
                break
            rep_ch = text[i + 1]
            rep_count = ord(text[i + 2])
            if rep_count > 0:
                out.append(rep_ch * rep_count)
            i += 3
            continue

        if code == 0x0C:
            out.append("\x1b[2J\x1b[H")
            i += 1
            continue

        if code != 0x16:
            out.append(ch)
            i += 1
            continue

        if i + 1 >= len(text):
            break
        cmd = ord(text[i + 1])
        i += 2

        if cmd == 0x01:
            if i >= len(text):
                break
            attr = ord(text[i])
            out.append(sgr_from_pc_attr(attr))
            i += 1
            continue

        if cmd == 0x02:
            out.append(BLINK)
            continue

        if cmd == 0x03:
            out.append("\x1b[1A")
            continue
        if cmd == 0x04:
            out.append("\x1b[1B")
            continue
        if cmd == 0x05:
            out.append("\x1b[1D")
            continue
        if cmd == 0x06:
            out.append("\x1b[1C")
            continue

        if cmd == 0x07:
            out.append("\x1b[K")
            continue

        if cmd == 0x08:
            if i + 1 >= len(text):
                break
            y = ord(text[i])
            x = ord(text[i + 1])
            out.append(f"\x1b[{y};{x}H")
            i += 2
            continue

        if cmd == 0x09:
            continue

    return "".join(out)


def disp_emu(
    text: str,
    remote_echo: bool = True,
    *,
    mode: Optional[str] = None,
    expand_ra_qbbs: Optional[bool] = None,
    ra_qbbs_ctx: Optional[RaqbbsContext] = None,
    ra_qbbs_f_map: Optional[Dict[str, str]] = None,
    ra_qbbs_k_map: Optional[Dict[str, str]] = None,
) -> None:
    if not remote_echo:
        return

    s = "" if text is None else str(text)

    cfg = _rt_config()
    if expand_ra_qbbs is None:
        expand_ra_qbbs = cfg.text.expand_ra_qbbs

    # Session-aware defaults.
    _get_runtime()
    if mode is None:
        df = _rt_dropfile()
        term = _rt_terminal()

        if df is not None:
            gm = (getattr(df, "graphics_mode", "") or "").strip().lower()
            if "ansi" in gm or gm in ("gr", "7e"):
                mode = "ansi"
            elif "rip" in gm:
                mode = "ansi"
            else:
                mode = "ascii"
        else:
            caps = [str(c).strip().lower() for c in (getattr(term, "capabilities", None) or [])]
            mode = "ansi" if ("ansi" in caps or "rip" in caps) else "ascii"

    if expand_ra_qbbs and ra_qbbs_ctx is None:
        ra_qbbs_ctx = ra_qbbs_context_from_dropfile(_rt_dropfile())

    if expand_ra_qbbs:
        s = expand_ra_qbbs_control_codes(s, ctx=ra_qbbs_ctx, f_map=ra_qbbs_f_map, k_map=ra_qbbs_k_map)

    m = str(mode).strip().lower()
    if m in ("ansi", "tty", "ascii"):
        s = _avatar_to_ansi(s)
    write(s)
