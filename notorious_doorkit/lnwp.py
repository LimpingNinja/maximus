"""
LNWP protocol abstraction for NotoriousPTY doors.
"""

from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple

DLE = 0x10
STX = 0x02
ETX = 0x03


def _escape_lnwp_payload(payload: bytes) -> bytes:
    """Escape DLE bytes in LNWP payload."""
    out = bytearray()
    for b in payload:
        out.append(b)
        if b == DLE:
            out.append(DLE)
    return bytes(out)


def quote_val(s: str) -> str:
    """Quote a value for LNWP payload."""
    safe = s.replace('"', "'").replace("\r", " ").replace("\n", " ")
    return f'"{safe}"'


def lnwp_payload(prm: str, val: Optional[str] = None, **kv: str) -> str:
    """
    Build an LNWP payload string.
    
    Args:
        prm: PRM parameter value
        val: Optional VAL parameter value
        **kv: Additional key-value pairs
        
    Returns:
        LNWP payload string
        
    Example:
        >>> lnwp_payload("SEND_MSG", "Hello", NODE_ID="2", TAG="CHAT")
        'LNWP V1 PRM=SEND_MSG VAL="Hello" NODE_ID="2" TAG="CHAT"'
    """
    parts: List[str] = ["LNWP", "V1", f"PRM={prm}"]
    if val is not None:
        parts.append(f"VAL={quote_val(val)}")
    for k, v in kv.items():
        parts.append(f"{k}={quote_val(v)}")
    return " ".join(parts)


def parse_lnwp_payload(payload: str) -> Dict[str, str]:
    """
    Parse an LNWP payload string into a dictionary.
    
    Args:
        payload: LNWP payload string
        
    Returns:
        Dictionary of key-value pairs
        
    Example:
        >>> parse_lnwp_payload('LNWP V1 PRM=NEW_MSG VAL="Hello" FROM_NODE="2"')
        {'PRM': 'NEW_MSG', 'VAL': 'Hello', 'FROM_NODE': '2'}
    """
    payload = payload.strip()
    if not payload:
        return {}

    tokens: List[str] = []
    current: List[str] = []
    in_quotes = False
    for ch in payload:
        if ch == '"':
            in_quotes = not in_quotes
            current.append(ch)
            continue
        if ch.isspace() and not in_quotes:
            if current:
                tokens.append(''.join(current))
                current.clear()
            continue
        current.append(ch)

    if current:
        tokens.append(''.join(current))

    result: Dict[str, str] = {}
    for tok in tokens:
        if '=' not in tok:
            continue
        k, v = tok.split('=', 1)
        key = k.strip().upper()
        val = v.strip()
        if len(val) >= 2 and val.startswith('"') and val.endswith('"'):
            val = val[1:-1]
        result[key] = val
    return result


@dataclass
class LnwpFrame:
    """LNWP frame with kind and payload."""
    kind: str
    payload: bytes


class LnwpFramer:
    """
    LNWP frame parser for extracting frames from byte stream.
    """
    
    def __init__(self) -> None:
        self._in_frame = False
        self._escape = False
        self._have_kind = False
        self._kind: Optional[int] = None
        self._payload = bytearray()
        self._pending_dle = False

    def feed(self, data: bytes) -> Tuple[List[LnwpFrame], bytes]:
        """
        Feed bytes to the framer.
        
        Args:
            data: Bytes to process
            
        Returns:
            Tuple of (frames, passthrough_bytes)
        """
        frames: List[LnwpFrame] = []
        passthrough = bytearray()

        for b in data:
            if not self._in_frame:
                if self._pending_dle:
                    if b == STX:
                        self._in_frame = True
                        self._have_kind = False
                        self._kind = None
                        self._payload.clear()
                        self._escape = False
                        self._pending_dle = False
                        continue
                    passthrough.append(DLE)
                    self._pending_dle = False

                if b == DLE:
                    self._pending_dle = True
                    continue

                passthrough.append(b)
                continue

            if not self._have_kind:
                self._kind = b
                self._have_kind = True
                continue

            if self._escape:
                if b == DLE:
                    self._payload.append(DLE)
                elif b == ETX:
                    kind_char = chr(self._kind or ord('?'))
                    frames.append(LnwpFrame(kind=kind_char, payload=bytes(self._payload)))
                    self._in_frame = False
                    self._have_kind = False
                    self._kind = None
                    self._payload.clear()
                else:
                    self._payload.append(DLE)
                    self._payload.append(b)
                self._escape = False
                continue

            if b == DLE:
                self._escape = True
                continue

            self._payload.append(b)

        return frames, bytes(passthrough)


@dataclass
class LnwpEvent:
    pass


@dataclass
class RawLnwpEvent(LnwpEvent):
    kind: str
    fields: Dict[str, str]


@dataclass
class NewMessageEvent(LnwpEvent):
    box: Optional[str]
    count: Optional[int]


@dataclass
class MessageEvent(LnwpEvent):
    fields: Dict[str, str]


@dataclass
class NodeInfoEvent(LnwpEvent):
    fields: Dict[str, str]


@dataclass
class NodesInfoEvent(LnwpEvent):
    nodes_info: Dict[int, Dict[str, str]]
    fields: Dict[str, str]
