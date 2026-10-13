# 56 — Haze is a row of its own
folder: scene
after: none
decisions: 0168, 0416, 0417

## Change
Bug 08: haze moves off the sky row to Rendering / Haze (0416 point 1, 0417 point 3). This card adds
the row; card 57 takes the fields off the sky. Built as the picture row is: read
`scene/include/scene/picture_component.h`, `scene/include/scene/picture_system.h`,
`scene/src/picture_component.c`, `scene/src/picture_system.c` and `scene/tests/picture.c` as the
pattern, and the header of `scene/include/scene/sky_component.h` for the haze fields' words today.

- `scene/include/scene/haze_component.h` (new): `voe_scene_haze`, a described struct of `haze` and
  `start`, both FLOAT32; its key; `_get`, `_count`, `_rows`, `_entities`. Header points: having the
  row is haze on, no row no haze; the first row in table order is used; works with or without a sky
  row; `haze` the metres past `start` at which half a surface's colour is haze, 0 none; `start` the
  metres within which no surface takes haze, 0 the fade from the eye (0414's fade); each finite and
  at least 0; the colour is the sky's horizon hue with a sky and the directional lights' colour
  without, its brightness always the directional lights' (0416 points 2 to 4), so a dark sky over a
  bright scene keeps bright haze; written only through `scene/haze_system.h`.
- `scene/include/scene/haze_system.h` (new): `voe_scene_haze_register(world, capacity)` (default
  row haze 1000, start 100, which is also the unsaid row; no needed type; menu "Rendering / Haze";
  the intent as its replace), `[[nodiscard]] bool voe_scene_haze_add(...)` asserting on a row the
  drain would refuse, `voe_scene_haze_intent { entity; haze; }`, `[[nodiscard]] bool
  voe_scene_haze_submit(...)`, `void voe_scene_haze_system_run(world)` refusing a number not finite
  or below 0 and keeping the row, one stderr line per refusal. Header: usage example and refusal
  paragraph as the picture's.
- `scene/src/haze_component.c`, `scene/src/haze_system.c` (new): the above, each with its header.
- `scene/tests/haze.c` (new): registration (menu path, no needed type, default row 1000 and 100,
  the two fields in order, FLOAT32); add and read back; a replace changes `haze` after a run; a NaN
  `haze`, a `haze` of −1, a `start` of −1 and an infinite `start` are each refused and keep the
  row; add on a dead entity is false.
- `scene/scene.md`, `scene/include/scene/scene.md`: an entry for each new header, beside the sky's;
  `scene/src/src.md`, `scene/tests/tests.md`: one each for the new files. Each at most 300
  characters.

## Done when
The test `scene/haze` passes after the folder's build.
