from __future__ import annotations

import os
from dataclasses import dataclass, replace
from functools import lru_cache
from pathlib import Path
from typing import Optional


_DEFAULT_EXPAND_RA_QBBS_ENVVAR = "NOTORIOUS_DOORKIT_EXPAND_RA_QBBS"
_DEFAULT_COLOR_DELIMITER_ENVVAR = "NOTORIOUS_DOORKIT_COLOR_DELIMITER"
_DEFAULT_COLOR_CHAR_ENVVAR = "NOTORIOUS_DOORKIT_COLOR_CHAR"
_DEFAULT_DETECT_CAPS_ENVVAR = "NOTORIOUS_DOORKIT_DETECT_CAPABILITIES"
_DEFAULT_CAPS_TIMEOUT_MS_ENVVAR = "NOTORIOUS_DOORKIT_CAPABILITY_TIMEOUT_MS"
_DEFAULT_DETECT_WINSIZE_ENVVAR = "NOTORIOUS_DOORKIT_DETECT_WINDOW_SIZE"
_DEFAULT_WINSIZE_TIMEOUT_MS_ENVVAR = "NOTORIOUS_DOORKIT_WINDOW_SIZE_TIMEOUT_MS"
_DEFAULT_CLEAR_ON_ENTRY_ENVVAR = "NOTORIOUS_DOORKIT_CLEAR_ON_ENTRY"

_GLOBAL_CONFIG_ENVVAR = "NOTORIOUS_DOORKIT_CONFIG"
_DOOR_CONFIG_ENVVAR = "NOTORIOUS_DOORKIT_DOOR_CONFIG"


_override_global_config_path: Optional[Path] = None
_override_door_config_path: Optional[Path] = None


def _bool_from_env(var_name: str, default: bool) -> bool:
    raw = os.getenv(var_name)
    if raw is None:
        return default
    val = str(raw).strip().lower()
    return val in ("1", "true", "yes", "on")


def _int_from_env(var_name: str, default: int) -> int:
    raw = os.getenv(var_name)
    if raw is None:
        return default
    try:
        return int(str(raw).strip())
    except Exception:
        return default


@dataclass(frozen=True)
class TextConfig:
    color_delimiter: str = "`"
    color_char: Optional[str] = None
    expand_ra_qbbs: bool = False


@dataclass(frozen=True)
class TerminalConfig:
    detect_capabilities: bool = False
    probe_timeout_ms: int = 250
    detect_window_size: bool = False
    window_size_timeout_ms: int = 250
    clear_on_entry: bool = True


@dataclass(frozen=True)
class UIThemeColors:
    text: str = "\x1b[36m"
    text_muted: str = "\x1b[37m"
    label: str = "\x1b[37m\x1b[44m"
    key: str = "\x1b[33m\x1b[1m"
    input: str = "\x1b[37m\x1b[40m"
    input_focused: str = "\x1b[37m\x1b[41m\x1b[1m"
    selected: str = "\x1b[37m\x1b[44m\x1b[1m"
    warning: str = "\x1b[33m\x1b[44m\x1b[1m"
    frame_border: str = "\x1b[37m\x1b[44m\x1b[1m"
    frame_fill: str = "\x1b[44m"


@dataclass(frozen=True)
class UIThemeConfig:
    hotkey_format: str = "[X]"
    colors: UIThemeColors = UIThemeColors()


@dataclass(frozen=True)
class UIConfig:
    theme: UIThemeConfig = UIThemeConfig()


@dataclass(frozen=True)
class DoorkitConfig:
    text: TextConfig = TextConfig()
    terminal: TerminalConfig = TerminalConfig()
    ui: UIConfig = UIConfig()


def _default_global_config_path() -> Optional[Path]:
    p = os.getenv(_GLOBAL_CONFIG_ENVVAR)
    if p:
        return Path(p)

    xdg = os.getenv("XDG_CONFIG_HOME")
    if xdg:
        return Path(xdg) / "notorious_doorkit" / "config.toml"

    home = Path.home()
    return home / ".config" / "notorious_doorkit" / "config.toml"


def _default_door_config_path() -> Optional[Path]:
    p = os.getenv(_DOOR_CONFIG_ENVVAR)
    if p:
        return Path(p)

    cwd_cfg = Path.cwd() / "config.toml"
    if cwd_cfg.exists() and cwd_cfg.is_file():
        return cwd_cfg

    return None


def _load_toml(path: Path) -> dict:
    try:
        import tomllib  # py3.11+
    except ModuleNotFoundError as e:
        raise RuntimeError("tomllib is required to read Doorkit TOML config") from e

    raw = path.read_bytes()
    data = tomllib.loads(raw.decode("utf-8", errors="replace"))
    if not isinstance(data, dict):
        return {}
    return data


def _config_from_dict(base: DoorkitConfig, data: dict) -> DoorkitConfig:
    if not isinstance(data, dict):
        return base

    text = base.text
    text_data = data.get("text")
    if isinstance(text_data, dict):
        if "color_delimiter" in text_data:
            v = text_data.get("color_delimiter")
            text = replace(text, color_delimiter="" if v is None else str(v))

        if "color_char" in text_data:
            v = text_data.get("color_char")
            v_s = "" if v is None else str(v)
            text = replace(text, color_char=(v_s if v_s else None))
        if "expand_ra_qbbs" in text_data:
            text = replace(text, expand_ra_qbbs=bool(text_data.get("expand_ra_qbbs")))

    terminal = base.terminal
    terminal_data = data.get("terminal")
    if isinstance(terminal_data, dict):
        if "detect_capabilities" in terminal_data:
            terminal = replace(terminal, detect_capabilities=bool(terminal_data.get("detect_capabilities")))
        if "probe_timeout_ms" in terminal_data:
            try:
                terminal = replace(terminal, probe_timeout_ms=int(terminal_data.get("probe_timeout_ms")))
            except Exception:
                pass
        if "detect_window_size" in terminal_data:
            terminal = replace(terminal, detect_window_size=bool(terminal_data.get("detect_window_size")))
        if "window_size_timeout_ms" in terminal_data:
            try:
                terminal = replace(terminal, window_size_timeout_ms=int(terminal_data.get("window_size_timeout_ms")))
            except Exception:
                pass
        if "clear_on_entry" in terminal_data:
            terminal = replace(terminal, clear_on_entry=bool(terminal_data.get("clear_on_entry")))

    ui = base.ui
    ui_data = data.get("ui")
    if isinstance(ui_data, dict):
        theme = ui.theme
        theme_data = ui_data.get("theme")
        if isinstance(theme_data, dict):
            if "hotkey_format" in theme_data:
                v = theme_data.get("hotkey_format")
                theme = replace(theme, hotkey_format="" if v is None else str(v))

            colors = theme.colors
            colors_data = theme_data.get("colors")
            if isinstance(colors_data, dict):
                for k in (
                    "text",
                    "text_muted",
                    "label",
                    "key",
                    "input",
                    "input_focused",
                    "selected",
                    "warning",
                    "frame_border",
                    "frame_fill",
                ):
                    if k in colors_data:
                        v = colors_data.get(k)
                        colors = replace(colors, **{k: "" if v is None else str(v)})
            theme = replace(theme, colors=colors)

        ui = replace(ui, theme=theme)

    return replace(base, text=text, terminal=terminal, ui=ui)


def _apply_env_overrides(cfg: DoorkitConfig) -> DoorkitConfig:
    text = cfg.text
    terminal = cfg.terminal

    if os.getenv(_DEFAULT_COLOR_DELIMITER_ENVVAR) is not None:
        text = replace(text, color_delimiter=str(os.getenv(_DEFAULT_COLOR_DELIMITER_ENVVAR) or ""))

    if os.getenv(_DEFAULT_COLOR_CHAR_ENVVAR) is not None:
        raw = str(os.getenv(_DEFAULT_COLOR_CHAR_ENVVAR) or "")
        text = replace(text, color_char=(raw if raw else None))

    if os.getenv(_DEFAULT_EXPAND_RA_QBBS_ENVVAR) is not None:
        text = replace(text, expand_ra_qbbs=_bool_from_env(_DEFAULT_EXPAND_RA_QBBS_ENVVAR, text.expand_ra_qbbs))

    if os.getenv(_DEFAULT_DETECT_CAPS_ENVVAR) is not None:
        terminal = replace(terminal, detect_capabilities=_bool_from_env(_DEFAULT_DETECT_CAPS_ENVVAR, terminal.detect_capabilities))
    if os.getenv(_DEFAULT_CAPS_TIMEOUT_MS_ENVVAR) is not None:
        terminal = replace(terminal, probe_timeout_ms=_int_from_env(_DEFAULT_CAPS_TIMEOUT_MS_ENVVAR, terminal.probe_timeout_ms))

    if os.getenv(_DEFAULT_DETECT_WINSIZE_ENVVAR) is not None:
        terminal = replace(terminal, detect_window_size=_bool_from_env(_DEFAULT_DETECT_WINSIZE_ENVVAR, terminal.detect_window_size))
    if os.getenv(_DEFAULT_WINSIZE_TIMEOUT_MS_ENVVAR) is not None:
        terminal = replace(
            terminal,
            window_size_timeout_ms=_int_from_env(_DEFAULT_WINSIZE_TIMEOUT_MS_ENVVAR, terminal.window_size_timeout_ms),
        )

    if os.getenv(_DEFAULT_CLEAR_ON_ENTRY_ENVVAR) is not None:
        terminal = replace(terminal, clear_on_entry=_bool_from_env(_DEFAULT_CLEAR_ON_ENTRY_ENVVAR, terminal.clear_on_entry))

    return replace(cfg, text=text, terminal=terminal)


def load_config(
    *,
    global_config_path: Optional[str | Path] = None,
    door_config_path: Optional[str | Path] = None,
) -> DoorkitConfig:
    cfg = DoorkitConfig()

    gpath = Path(global_config_path) if global_config_path is not None else _default_global_config_path()
    if gpath is not None and gpath.exists() and gpath.is_file():
        cfg = _config_from_dict(cfg, _load_toml(gpath))

    dpath = Path(door_config_path) if door_config_path is not None else _default_door_config_path()
    if dpath is not None and dpath.exists() and dpath.is_file():
        cfg = _config_from_dict(cfg, _load_toml(dpath))

    cfg = _apply_env_overrides(cfg)
    return cfg


def configure(
    *,
    global_config_path: Optional[str | Path] = None,
    door_config_path: Optional[str | Path] = None,
) -> DoorkitConfig:
    global _override_global_config_path
    global _override_door_config_path

    _override_global_config_path = Path(global_config_path) if global_config_path is not None else None
    _override_door_config_path = Path(door_config_path) if door_config_path is not None else None

    get_config.cache_clear()
    return get_config()


@lru_cache(maxsize=1)
def get_config() -> DoorkitConfig:
    return load_config(
        global_config_path=_override_global_config_path,
        door_config_path=_override_door_config_path,
    )
