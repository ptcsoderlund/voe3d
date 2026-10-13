# 68 — The game frame test passes the eye to the sky
folder: game/tests
after: 64, 65, 66, 67
decisions: 0168, 0418, 0419

## Change
Card 66 made `voe_3d_draw_system_sky` take the eye; `game/tests/frame.c`'s haze case calls it.
A game world needs no other code: the Haze row's new field is described, so scene text, the
Inspector, saving, Play and Ship carry it (card 65). Read its comment in
`3d/include/3d/draw_system.h` and the header of `game/tests/frame.c`.

- `game/tests/frame.c`, the haze case: pass the origin as the eye; the Haze row adds `height`
  1000 and the case also checks `haze_top` reads 1000. The comment above that case's function
  names it; the file's header stays at most 60 lines.
- `game/tests/tests.md` (`frame.c`) only if it names the haze case's fields: follow, at most 300
  characters.

## Done when
The test `game/frame` passes after the folder's build.

The human's, in the editor on `lumberhill_project` with a Sky, a directional light and Rendering /
Haze (feature.md's How to test step 7):
1. The Haze shows Height 1000. Set the haze close and dense (small haze, small start): the whole
   sky turns to fog and the sun's disc and glow are gone.
2. Make it light (large haze): the horizon is hazy, the sky clears towards the top, and the sun
   shows through.
3. Set Height below the camera and look up: the sky is clear; look down past the horizon: haze.
4. Raise and lower Height with the camera inside the layer: far ground fades by distance just
   the same. Remove the Sky: the backdrop fades into the haze the same way. Remove the Haze:
   nothing fades and the sky is clear.
5. A shiny metal sphere still reflects the clear sky. Open `examples/tank_game`: as before. Save,
   close and reopen; Play; Ship: the same picture in all three.
