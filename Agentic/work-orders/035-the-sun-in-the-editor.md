# 035 — The sun in the editor

## What
The sun (the scene's directional light) can be seen and handled in the editor like anything else.
It shows in the scene views as a marker with an arrow pointing the way the light shines. Clicking
the marker selects it. A selected thing can be turned with a rotate gizmo: a key switches the gizmo
between move and rotate, and the top bar shows which one is on. Turning the sun moves every shadow
in both views while the drag goes on. The Inspector shows the sun's settings and changes them live:
its colour, its strength, and the colour and strength of the fill light that keeps shadowed sides
from going black. Every change can be undone, is saved with the scene, and looks the same when the
game is played.

## Why
Milestone 2 of 0268. The sponsor needs to light the tank game's levels by eye, not by typing
numbers into a text file.

## How to test
1. Open `examples/coin_game` in the editor. The sun shows as a marker with an arrow in both views.
2. Click the marker. The sun is selected, outlined, and its settings are in the Inspector.
3. Switch the gizmo to rotate. The top bar says rotate, and rings show on the sun. Drag a ring. The
   shadows swing round in both views as you drag.
4. Switch back to move. The arrows come back. Rotate works on any other thing too: select the player
   and turn it.
5. Change the sun's colour to orange and its strength to half. The scene goes dim and orange at once.
   Raise the fill light. The shadowed sides get lighter, and the shadows are still there.
6. Undo the steps one at a time. Each one goes back.
7. Make a change, save, close the editor and open the project again. The change is still there.
8. Press Play. The game is lit the same way as the editor.
