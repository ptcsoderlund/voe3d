# 01 — The sun's colours have no colour picker

## Seen
No colour picker shows on `colour` and `fill_colour` on the light component.

## Expected
Both `colour` and `fill_colour` in the Inspector open the colour picker, just as other colour fields in
the editor do. Picking a colour changes the scene at once, can be undone, and is saved with the scene
(steps 5–7 of the feature's How to test).

## How to reproduce
1. Open `examples/coin_game` in the editor.
2. Click the sun's marker to select the sun.
3. In the Inspector, look at the light component's `colour` and `fill_colour`.
4. There is no colour picker on either of them.
