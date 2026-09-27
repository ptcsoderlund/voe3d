# 09 — Dev's sun laps by its transform
folder: dev
decisions: 0168, 0273

## Change
Needs cards 01–03.

- `dev/src/motion.h`, `dev/src/motion.c`: `voe_dev_sunlight(sun, seconds)` becomes
  `voe_scene_transform voe_dev_sun_pose(float seconds)`: the lap's direction as now, turned
  into a rotation by `voe_scene_light_facing`, at a position that keeps the marker out of the
  picture's way (above the origin, e.g. (0, 4, 0)), scale one. Its comment says the sun is
  turned by its transform (0273); the paragraph on why it moves stays.
- `dev/src/startup.c`: the sun entity gets `voe_scene_transform_add` with
  `voe_dev_sun_pose(0.0f)` before `voe_scene_light_add` of the colour and intensity the old
  intent carried, no fill. The transform table is already registered first.
- `dev/src/main.c`: where the light intent was submitted each frame, submit a
  `voe_scene_transform_intent` for `program.sun` with `voe_dev_sun_pose(seconds)` (as the
  eye's pose is submitted); keep the light system's run. Add no lines beyond that; the file is
  near 800.
- `dev/src/src.md`: the `motion` lines say what `voe_dev_sun_pose` is.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_dev` exits 0, and
`grep -n "voe_dev_sunlight\|\.direction = {" dev/src/*.c dev/src/*.h` prints nothing.
