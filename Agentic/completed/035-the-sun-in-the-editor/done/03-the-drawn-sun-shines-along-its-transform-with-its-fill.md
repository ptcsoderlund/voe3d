# 03 — The drawn sun shines along its transform, with its fill
folder: 3d
decisions: 0168, 0273

## Change
Needs cards 01 (scene) and 02 (render). `3d`'s public surface is unchanged.

- `3d/src/draw_system.c`, `voe_3d_draw_system_light`: the render light's `direction` is
  `voe_scene_light_direction` of the light entity's transform rotation, (0, 0, −1) when it has
  no transform; `fill` is `fill_colour × fill_intensity`; colour and intensity as before. Fix
  its comment that says the direction arrives unit length from the row.
- `3d/include/3d/draw_system.h`: the paragraph on `voe_3d_draw_system_light` says where the
  direction and the fill come from.
- Every test that registers or adds a light: `3d/tests/no_light.c`, `import.c`, `pick.c`,
  `draw_system.c`, `shadows.c`, `outline.c`, `far.c`, `panel.c`. Register the transform table
  before the light's (its register now asserts on it). A light that had `.direction = d` gets a
  transform whose rotation is `voe_scene_light_facing(d)` (position anywhere, scale one), so
  every picture stays as it was.
- `3d/tests/no_light.c`: add two checks — a light whose transform is turned −π/2 about X
  frames a direction of (0, −1, 0) within 1e-5, and a fill colour (1, 0.5, 0) at intensity 0.4
  frames `fill` (0.4, 0.2, 0). Update its line in `3d/tests/tests.md`.
- `3d/tests/shadows.c`: add a case that the floor's shadowed patch under the cube, with fill
  0.2 white, reads lighter than without it and still darker than the lit floor beside it.
  Update its line in `3d/tests/tests.md`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_3d $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_3d_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R "^3d/"`
exits 0.
