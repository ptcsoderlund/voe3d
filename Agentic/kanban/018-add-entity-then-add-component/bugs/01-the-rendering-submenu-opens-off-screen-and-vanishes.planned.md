# 01 — The Rendering submenu opens off screen and vanishes at once

## Seen
"When I click "Add component" -> "Rendering >" a dropdown pops up to the right of the button. There is no space
there, so it is partially out of the screen, and it only flickers. It disappears right away; I can see it for a
small part of a second."

Two faults:
1. The submenu opens to the right of the Rendering row although the Inspector sits at the right edge of the
   window, so part of it is outside the window.
2. The submenu closes on its own a fraction of a second after it opens, without the person pressing anything
   else.

## Expected
1. When there is no room to the right of the list, the submenu opens to the left of it, wholly inside the
   window. It opens to the right only when it fits there.
2. The submenu stays open until the person picks a type in it, opens another group, presses outside the menu,
   or presses Escape. Picking Light or Shape in it adds that component.

## How to reproduce
1. Open the editor on a project, with the Inspector at the right edge of the window as it is by default.
2. Press Add entity.
3. Press Add component.
4. Press "Rendering >".
5. The submenu appears to the right, partly outside the window, and is gone within a fraction of a second.
