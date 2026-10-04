# 0356 — The splash is a PNG read by app and drawn by game's starting frame
date: 2026-10-04
by: planner

## Decision
For 057, carrying out 0346:
1. **Reading.** `app/picture.h` reads a PNG file into a COLOUR texture, CLAMP sampled, with its
   width and height. `app` already reaches `platform`, `assets` and `render`, so no new edge.
   The texture samples NEAREST, as every texture does (render has no linear filtering).
2. **Drawing.** `game/starting.h`'s two calls take a splash picture, NULL for 055's plain screen.
   With one, the frame is: the picture's top-left texel stretched over the whole surface (that
   texel is the image's edge colour); the whole picture anchored centred at the largest size that
   fits the surface without changing its aspect; a GROUND panel with the line in normal text,
   anchored at the bottom middle, its lower edge 8 mm above the surface's.
3. **The engine's copy** is `game/src/splashscreen.png`, copied unchanged from `engine_assets/`.
   The editor reads it at run time from the engine source its build names (`toolchain.h`'s
   `VOE_TOOLCHAIN_ENGINE`), so a missing copy shows the plain screen with no rebuild. It keeps
   the texture for scene loads and gives it back before the device closes.
4. **A game's copy.** `cmake/game.cmake` copies the project's `Assets/splashscreen.png`, or the
   engine's copy when the project has none, to `splashscreen.png` beside the program, and
   installs it there. The run reads it there, uses it for the start and gives the texture back
   after; a game has no scene change yet, so nothing else draws it.
5. **Editor loads.** New and Open's Confirm no longer replace the project in the frame they are
   asked: they mark a load due, and the next frame draws one splash frame with the line
   "Loading scene..." and then does the load, so the splash is seen before the work.

## Reasoning
`game` may not name `assets`, and `app` may; a picture-to-texture read is a frame loop's need
shared by both programs. The top-left texel is the edge colour without computing anything, and
it stays right for a project's own image. Reading the engine's copy from the source tree at run
time makes 057's "rename it and the plain screen shows" hold without build logic. Alternatives:
a new `game` to `assets` edge (an architecture change for one read); `#embed` in the editor
(0346 rejects embedding, and a missing file would break the build); averaging the border
pixels (more code, same answer for the engine's image).

## Replaces
nothing. Carries out 0346; amends 0345 point 2 (the starting frame takes a splash).
