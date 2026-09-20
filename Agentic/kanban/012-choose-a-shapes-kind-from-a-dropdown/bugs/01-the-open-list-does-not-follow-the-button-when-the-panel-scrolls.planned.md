# 01 — The open list does not follow the button when the panel scrolls

## Seen
"The dropdown on kind is getting locked. So when i scroll up and down, the popup with the items in dropdown
stays and do not follow the trigger buttons position."

Open the kind dropdown in the Inspector and scroll the panel. The Shape section and its dropdown button move
with the scroll, but the open list of kinds stays nailed where it first appeared. The list ends up next to,
above or on top of the wrong field, and no longer points at the button it belongs to.

## Expected
The open list stays glued under its trigger button and moves with it as the panel scrolls, exactly as if it
were drawn with the button. If the button scrolls out of the panel, the list is clipped away with it, the same
as any other panel content — it does not float on over the rest of the editor.

## How to reproduce
1. Open the editor on a project with enough entities or components that the Inspector panel scrolls.
2. Select an entity with a Shape and open the kind dropdown.
3. Scroll the Inspector up and down with the wheel while the list is open.
4. The list stays put while the button moves away from under it.
