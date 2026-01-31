from __future__ import annotations

import re
from functools import lru_cache
from typing import Optional

from .config import get_config
from .text import render_opendoors_text


_TOKEN_RE = re.compile(r"\{([A-Za-z_][A-Za-z0-9_]*)\}")


@lru_cache(maxsize=1)
def _ansi_token_map() -> dict[str, str]:
    from . import ansi as ansi_mod

    out: dict[str, str] = {}
    for k, v in vars(ansi_mod).items():
        if not k or not k.isupper():
            continue
        if isinstance(v, str):
            out[k] = v
    return out


def expand_doorkit_style_tokens(text: str) -> str:
    if not text:
        return "" if text is None else str(text)

    mp = _ansi_token_map()

    def repl(m: re.Match[str]) -> str:
        key = m.group(1).upper()
        v = mp.get(key)
        return m.group(0) if v is None else v

    return _TOKEN_RE.sub(repl, str(text))


def resolve_style(
    value: Optional[str],
    default: str,
    *,
    delimiter: Optional[str] = None,
    color_char: Optional[str] = None,
    expand_ra_qbbs: Optional[bool] = None,
) -> str:
    raw = default if value is None else value
    raw_s = "" if raw is None else str(raw)

    raw_s = expand_doorkit_style_tokens(raw_s)

    cfg = get_config()
    if delimiter is None:
        delimiter = cfg.text.color_delimiter
    if color_char is None:
        color_char = cfg.text.color_char
    if expand_ra_qbbs is None:
        expand_ra_qbbs = cfg.text.expand_ra_qbbs

    return render_opendoors_text(
        raw_s,
        delimiter=delimiter,
        color_char=color_char,
        expand_ra_qbbs=bool(expand_ra_qbbs),
    )
