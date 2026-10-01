# 23 — The bounce is strong enough to see in the tank scene
folder: render
after: 22
decisions: 0168, 0307, 0308, 0310, 0311

## Change
Bug 02: at sun 1 the tank scene shows no tint at the purple box's foot.
0310 raises the bounce's strength; 0311 says where. Read
`render/shaders/lighting.slangh`, `render/shaders/shaders.md`,
`examples/tank_game/main.scene` (the Ground, the Sun, Obstacle at
(−4.09, 5, 6.0), 2 × 10 × 1, base (0.45, 0.14, 0.49)), and, for projecting
into the view picture, `editor/src/view.h` and `editor/src/view.c` (where
`voe_editor_views_focus_camera` puts view 0's eye, its yaw, pitch and lens).

- Measure first. Capture the scene as card 22 made possible:
  `build/debug/editor/voe_editor examples/tank_game --capture build/debug/w.png --frames 60 --capture-view build/debug/sun1.png`;
  and at sun π: copy `examples/tank_game` to `build/debug/tank_sun_pi`, set
  its Sun's `intensity` line to 3.14159, capture it the same way to
  `build/debug/sunpi.png`. In each view picture take ground pixels 1 m,
  4 m and 8 m out from the Obstacle's sunlit face, on ground the sun
  reaches (project the points from view 0's eye; any reader, `python3`
  with PIL is on this machine). The render line the editor prints names
  the card: it must be the discrete RTX 4070, not llvmpipe.
- `lighting.slangh`: `VOE_BOUNCE_GAIN`, applied to E/π before the fill
  floor's max; set it to the lowest whole number at which, in both
  pictures, blue at 1 m ≥ blue at 8 m + 12, and blue at 4 m lies between
  them. Its header's formula and a line on the gain (0310, 0311).
- If at gain 50 blue at 1 m is still within 2 of 8 m, the fault is not
  strength: set nothing, block with the measured pixels at gains 1 and 50.
- `shaders.md`: the `lighting.slangh` entry names the gain.
- A render test (`render/tests/bounce.c`, `bounce_scene.c`) that the gain
  moves past its threshold: re-measure, keep its near-over-far claim, its
  header says the gain; `render/tests/tests.md` if an entry changes.

## Done when
The folder's tests pass. The two captures above, rerun after the build,
meet the blue condition; the commit message gives the gain, the card from
the render line, and the 1 m, 4 m and 8 m pixels of both pictures.
