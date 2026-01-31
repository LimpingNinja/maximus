from __future__ import annotations

from dataclasses import dataclass
from typing import Optional, Tuple, TYPE_CHECKING

from .config import DoorkitConfig, get_config

if TYPE_CHECKING:
    from .door import DoorSession, TerminalInfo
    from .dropfiles import DropfileInfo


@dataclass(frozen=True)
class Runtime:
    config: DoorkitConfig
    session: "DoorSession"

    @property
    def dropfile(self) -> Optional["DropfileInfo"]:
        return self.session.dropfile

    @property
    def terminal(self) -> "TerminalInfo":
        return self.session.terminal

    @property
    def username(self) -> str:
        return _resolve_username(self.session, default="User")

    @property
    def screen_size(self) -> Tuple[int, int]:
        return _resolve_screen_size(self.session, default=(24, 80))

    @property
    def screen_rows(self) -> int:
        return int(self.screen_size[0])

    @property
    def screen_cols(self) -> int:
        return int(self.screen_size[1])


_CURRENT_RUNTIME: Optional[Runtime] = None

_TERMINAL_FD: Optional[int] = None

_LNWP_FD: int = 0


def set_terminal_fd(fd: Optional[int]) -> None:
    global _TERMINAL_FD
    _TERMINAL_FD = fd


def clear_terminal_fd() -> None:
    global _TERMINAL_FD
    _TERMINAL_FD = None


def get_terminal_fd(*, default: Optional[int] = None) -> Optional[int]:
    fd = _TERMINAL_FD
    if fd is None:
        return default
    return fd


def set_lnwp_fd(fd: Optional[int]) -> None:
    global _LNWP_FD
    try:
        v = int(fd) if fd is not None else 0
    except Exception:
        v = 0
    _LNWP_FD = v if v > 0 else 0


def clear_lnwp_fd() -> None:
    global _LNWP_FD
    _LNWP_FD = 0


def get_lnwp(*, default: int = 0) -> int:
    fd = int(_LNWP_FD)
    if fd <= 0:
        return int(default)
    return fd


def init_runtime(session: "DoorSession", *, config: Optional[DoorkitConfig] = None) -> Runtime:
    global _CURRENT_RUNTIME
    cfg = get_config() if config is None else config
    _CURRENT_RUNTIME = Runtime(config=cfg, session=session)
    return _CURRENT_RUNTIME


def clear_runtime() -> None:
    global _CURRENT_RUNTIME
    _CURRENT_RUNTIME = None
    clear_terminal_fd()
    clear_lnwp_fd()


def get_runtime() -> Runtime:
    rt = _CURRENT_RUNTIME
    if rt is None:
        raise RuntimeError("Door runtime not initialized. Call Door.start(...) or Door.start_local(...) first.")
    return rt


def get_runtime_config() -> DoorkitConfig:
    return get_runtime().config


def get_session() -> "DoorSession":
    return get_runtime().session


def get_dropfile() -> Optional["DropfileInfo"]:
    return get_runtime().dropfile


def get_terminal() -> "TerminalInfo":
    return get_runtime().terminal


def get_username() -> str:
    return get_runtime().username


def get_screen_size() -> Tuple[int, int]:
    return get_runtime().screen_size


def _resolve_username(session: "DoorSession", *, default: str) -> str:
    u = getattr(session, "username", None)
    if u and str(u).strip():
        return str(u).strip()

    df = getattr(session, "dropfile", None)
    if df is not None:
        alias = getattr(getattr(df, "user", None), "alias", None)
        if alias and str(alias).strip():
            return str(alias).strip()
        full = getattr(getattr(df, "user", None), "full_name", None)
        if full and str(full).strip():
            return str(full).strip()

    return default


def _resolve_screen_size(session: "DoorSession", *, default: Tuple[int, int]) -> Tuple[int, int]:
    term = getattr(session, "terminal", None)
    rows = getattr(term, "rows", None) if term is not None else None
    cols = getattr(term, "cols", None) if term is not None else None

    if rows is None or cols is None:
        df = getattr(session, "dropfile", None)
        if df is not None and rows is None:
            sr = getattr(df, "screen_rows", None)
            if sr:
                rows = int(sr)

    if rows is None:
        rows = int(default[0])
    if cols is None:
        cols = int(default[1])

    return (rows, cols)
