"""
User activity string management.
"""

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .door import Door


def set_activity(door: 'Door', activity: str) -> None:
    """
    Set user's activity string.
    
    Args:
        door: Door instance
        activity: Activity description
        
    Example:
        >>> from notorious_doorkit.door import Door
        >>> from notorious_doorkit.activity import set_activity
        >>> door = Door()
        >>> set_activity(door, "Playing Trivia")
    """
    door.set_activity(activity)


def clear_activity(door: 'Door') -> None:
    """
    Clear user's activity string.
    
    Args:
        door: Door instance
    """
    door.set_activity("")
