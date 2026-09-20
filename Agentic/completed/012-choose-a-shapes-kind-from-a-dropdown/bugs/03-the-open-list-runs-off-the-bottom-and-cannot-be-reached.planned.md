# 03 — The open list runs off the bottom and cannot be reached

## Seen
"The dropdown dont detect overflow. I want to be able to scroll it and select bottom item when out of screen.
If overflow is really heavy, maybe dropdown list should spawn above button instead?"

A dropdown near the bottom of the Inspector opens downward regardless of how little room is left. The list runs
past the bottom of the panel and the items at its end are cut off, with no way to scroll to them — the bottom
kind simply cannot be picked from that position.

## Expected
An open list always shows all of its items or gives you a way to reach them.

- If it fits below the button, it opens below, as now.
- If it does not fit below but fits above, it opens above the button instead, its bottom edge against the
  button's top edge, and picking works the same way.
- If it fits neither way, it opens on the side with more room, capped to that room, and scrolls inside itself:
  the wheel over the list moves the items within it, the button stays put and the list stays the size it is,
  and every item can be reached and picked, the last one included.

The wheel over an open list scrolls the list, not the panel behind it. The choice of above or below is made
each frame the list is open, so a list that opened downward flips when the panel scrolls the button near the
bottom edge — it never ends up half cut off.

## How to reproduce
1. Open the editor on a project with enough entities or components that the Inspector panel scrolls.
2. Scroll the Inspector so an entity's Shape section sits near the very bottom of the panel, with only a row or
   two of room left under the kind's dropdown button.
3. Open the kind dropdown.
4. The list is cut off at the panel's bottom edge. Cylinder is not on screen and there is no way to scroll to
   it or pick it.
