# 01 — The views' border cannot be seen

## Seen
"022 is working good. However, the line is either invisible or the same color as bg. I need to see it to
drag it. Should probably be more than one color to guarantee contrast."

## Expected
The border between the two scene views is always visible, whatever the theme and whatever the scenes on
either side show. It is drawn in more than one colour — a dark and a light stripe side by side — so at least
one of them stands out against anything next to it. It stays visible while it is dragged, and the drag area
is no smaller than it is today.

## How to reproduce
1. Open the editor with any theme, including a monochrome one.
2. Look for the border between the two scene views: it cannot be told apart from the background.
3. Switch themes and move the scene camera so a view shows light and then dark content; the border should
   be visible in every case.
