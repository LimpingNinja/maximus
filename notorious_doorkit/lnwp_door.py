"""LNWP-aware door context.

This module provides a door runtime that can speak LNWP (NotoriousPTY control
plane). It intentionally lives outside `notorious_doorkit.lnwp`, which is
protocol-only.

Use `notorious_doorkit.door.Door` for a pure runtime/session context without any
protocol awareness.
"""

from __future__ import annotations

from typing import Optional

from .door import Door
from .runtime import get_lnwp, get_session, get_terminal_fd


class LnwpDoor(Door):
    """Backwards-compatible alias for a Door with optional LNWP helpers."""

    def __init__(self, door_key: Optional[str] = None):
        super().__init__()
        if door_key is not None:
            self.door_key = door_key

    @property
    def terminal_fd(self) -> int:
        fd = get_terminal_fd(default=None)
        if fd is not None and int(fd) > 0:
            return int(fd)
        return 1

    @property
    def control_fd(self) -> int:
        return int(get_lnwp())

    @property
    def input_fd(self) -> int:
        return int(get_lnwp())

    @property
    def door32_mode(self) -> bool:
        try:
            s = get_session()
        except Exception:
            return False
        df = getattr(s, "dropfile", None)
        p = getattr(df, "path", None)
        name = getattr(p, "name", "")
        return str(name).lower() == "door32.sys"
