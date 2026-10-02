# 02 — A scene saved under 047 keeps its sun's bounce, and the cook carries the strength
folder: authoring
after: 01
decisions: 0168, 0326

## Change
0326 point 1, as the reader and the cook see it after card 01; no source change is expected,
only the tests that name the light's fields.
- `authoring/tests/scene_cook.c`: the expected cooked text of the sun gains the new last field,
  `.bounce_strength` as the cook spells a float, after `.cast_shadows = true`; give the
  test's sun a strength of its own (say 0.5) so the line proves the value travels.
- `authoring/tests/scene_read_unsaid.c`: a new case beside `test_light_cast_shadows`: a light
  section as 047 saved it, `bounces = 1` and no `bounce_strength` key, read into a world with
  scene's real light registered, reads bounces 1 and strength 1 (How to test step 9). The file
  header names the case.
- `authoring/tests/tests.md`: the two entries, if their wording no longer covers them.

## Done when
`ctest --test-dir build/debug -R "^authoring/scene_(cook|read_unsaid)$"` passes.
