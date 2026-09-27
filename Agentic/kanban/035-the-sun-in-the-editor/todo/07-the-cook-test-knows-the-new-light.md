# 07 — The cook test knows the new light
folder: authoring
decisions: 0168, 0273

## Change
Needs card 01. The cook and the scene text go through field descriptions, so only the test
names the light's fields.

- `authoring/tests/scene_cook.c`: its two `voe_scene_light_add` calls drop `.direction`
  (give a fill instead, e.g. `fill_colour` white at `fill_intensity` 0.25, so a fill field is
  cooked), and the expected cooked text at the light's line spells `colour`, `intensity`,
  `fill_colour` and `fill_intensity` in the order the header declares them. If the world the
  test builds registers the light before the transform, swap them: the light's register now
  asserts on a transform table.
- Any other test under `authoring/tests/` that registers a light: the same order fix
  (grep `voe_scene_light_register`).

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_authoring $(ninja -C
build/debug -t targets all | grep -oE "^voe_test_authoring_[A-Za-z0-9_]+") && ctest --test-dir
build/debug -R "^authoring/"` exits 0.
