# 18 — A fading model is drawn see-through, and a gone one not at all
folder: 3d
after: 16, 17
decisions: 0168, 0336

## Change
0336 point 3. Read `3d/src/draw_system.c`, `3d/src/draw_group.h`, `3d/src/draw_group.c`,
`3d/src/draw_shadows.c`, the header of `3d/src/draw_bounce.h`, the run's comment in
`3d/include/3d/draw_system.h` (the model paragraph and "A DRAWN OBJECT'S COLOUR", near line
500), `3d/include/3d/model_component.h`, `3d/include/3d/models.h`, `3d/tests/models.c` (a model
loaded into a store and drawn: the pattern), `3d/tests/draw_water.c` (a picture's centre read
and compared: the pattern), `3d/tests/model_data.inc`, `3d/tests/tests.md`, `3d/src/src.md`,
`3d/include/3d/3d.md`.

- `draw_system.c`: a model row's fade read once per row. At or above 1: none of its parts is
  drawn. Above 0 and below 1: each part goes to the world's blended group, sorted with the rest,
  with its `faded` record and an object colour of (1, 1, 1, 1 − fade); the group's room counts
  those parts. At or below 0, or not a number: as today. Header points to match.
- `draw_group.h`, `draw_group.c`: if a held entry cannot yet carry a colour's alpha other than 1
  for a model part, it gains one; nothing else about the groups changes.
- `draw_shadows.c`: the model caster walk skips a row with `fade` at or above 1 (the bounce map
  shares the walk); a fading one casts as today. The header's who-casts paragraph says so.
- `draw_system.h`: the run's model paragraph says a fading model is blended at 1 − fade with its
  parts' twins and a gone one is not drawn; the colour paragraph names the fade's alpha.
- `3d/tests/draw_model_fade.c`, new, found by the build: a store with the hand-built `.glb`
  loaded, a model row on a ground cube's centre seen from above. The picture's centre with
  `fade` 0, 0.5 and 1: 0.5 differs from both, 1 equals the frame with no model row. With `fade`
  1 the shadow passes draw as with no model row. Skips without a graphics card.
- `3d/tests/tests.md` (the new test), `3d/src/src.md` (`draw_system.c`, `draw_shadows.c`,
  `draw_group.c` where changed), `3d/include/3d/3d.md` (`draw_system.h`): a phrase each.

## Done when
`ctest --test-dir build/debug -R '^3d/(draw_model_fade|models|shadows|draw_system)$'` passes;
on a machine without a graphics card these skip, and building `voe_test_3d_draw_model_fade` is
the proof.
