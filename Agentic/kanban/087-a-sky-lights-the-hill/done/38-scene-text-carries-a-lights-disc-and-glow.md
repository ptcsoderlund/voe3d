# 38 — Scene text carries a light's disc and glow
folder: authoring
after: 37
decisions: 0168, 0412

## Change
Card 37 gave `voe_scene_light` two fields after `bounce_strength`; the writer, reader and cook are
generic (they walk the description), so only the tests that spell a light out change. Read the
headers of `authoring/tests/scene_cook.c`, `authoring/tests/scene_lights.c` and
`authoring/tests/scene_read_unsaid.c`.

- `authoring/tests/scene_cook.c`: the sun is added with `disc_size` 2 and `glow_size` 0.5, and the
  expected cooked source carries `.disc_size` and `.glow_size` after `.bounce_strength`, in the
  hex-float form the cook already writes.
- `authoring/tests/scene_lights.c`: the sun and the moon carry distinct disc and glow sizes (the
  moon disc 0.5, glow 0); the row comparison checks both fields.
- `authoring/tests/scene_read_unsaid.c`: the 047 light, which says neither field, reads disc 1.5
  and glow 0.3, the light's unsaid row, so an old scene shows a sun under a sky.
- `authoring/tests/tests.md`: the three entries changed where what they say changed, each one
  sentence of at most 300 characters.

## Done when
The tests `authoring/scene_cook`, `authoring/scene_lights` and `authoring/scene_read_unsaid` pass
after the folder's build.
