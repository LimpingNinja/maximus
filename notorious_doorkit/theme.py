from __future__ import annotations

import copy
from dataclasses import dataclass, field
from typing import Optional

from .config import get_config
from .style import resolve_style


_CURRENT_THEME: Optional["Theme"] = None


@dataclass
class ThemeColors:
    text: str = ""
    text_muted: str = ""
    label: str = ""
    key: str = ""
    input: str = ""
    input_focused: str = ""
    selected: str = ""
    warning: str = ""
    frame_border: str = ""
    frame_fill: str = ""


@dataclass
class ThemeFormat:
    hotkey: str = "[X]"
    press_enter: str = ""
    press_any_key: str = ""


@dataclass
class Theme:
    colors: ThemeColors = field(default_factory=ThemeColors)
    format: ThemeFormat = field(default_factory=ThemeFormat)

    def color(self, role: str, override: Optional[str] = None) -> str:
        default = getattr(self.colors, role)
        return resolve_style(override, default)


def set_current_theme(theme: Optional[Theme]) -> None:
    global _CURRENT_THEME
    _CURRENT_THEME = theme


def clear_current_theme() -> None:
    set_current_theme(None)


def get_current_theme() -> Theme:
    t = _CURRENT_THEME
    return build_theme() if t is None else t


def build_theme(*, source: Optional[Theme] = None) -> Theme:
    if source is not None:
        return copy.deepcopy(source)

    cfg_theme = get_config().ui.theme

    return Theme(
        colors=ThemeColors(
            text=cfg_theme.colors.text,
            text_muted=cfg_theme.colors.text_muted,
            label=cfg_theme.colors.label,
            key=cfg_theme.colors.key,
            input=cfg_theme.colors.input,
            input_focused=cfg_theme.colors.input_focused,
            selected=cfg_theme.colors.selected,
            warning=cfg_theme.colors.warning,
            frame_border=cfg_theme.colors.frame_border,
            frame_fill=cfg_theme.colors.frame_fill,
        ),
        format=ThemeFormat(
            hotkey=cfg_theme.hotkey_format,
            press_enter="",
            press_any_key="Press any key to continue...",
        ),
    )
