# 15 — A 3d frame carries each point light's falloff
folder: 3d
after: 13, 14
decisions: 0168, 0321, 0322

## Change
3d fills a pass's lights from the table's intensity and falloff now that a point light has no
flash (0321 point 2, 0322 points 1 and 5). Read the headers of
`scene/include/scene/point_light_component.h` and `scene/include/scene/point_light_system.h`, the
`voe_render_point_light` comment in `render/include/render/device.h`, and the files below.

- `3d/src/draw_point_lights.c`: each kept light's colour is colour × the row's `intensity` (no
  more `voe_scene_point_light_strength`), and the record's `falloff` is the row's; an intensity of
  0 is still left out. Header comment to match.
- `3d/include/3d/draw_system.h`: `voe_3d_draw_system_point_lights`' comment says colour times
  intensity, falloff as authored, a light of intensity 0 left out; the flash wording goes.
- `3d/tests/point_lights.c`: the flash cases (never flashed left out, in after a flash) go, with
  every `voe_scene_point_light_flash_submit`; every `voe_scene_point_light_system_run` call takes
  the world alone, or goes where it only made glow rows; every light it adds sets falloff 1. New:
  a light of intensity 0 is left out; a light of falloff 2.5 gives a record of falloff 2.5. The
  picture case is unchanged otherwise.
- `3d/tests/point_light_marker.c`: the light it adds sets falloff 1, so `add` does not assert.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: the draw_system.h, draw_point_lights.c and
  point_lights.c entries say intensity and falloff, not strength or flash. Each at most 300
  characters.

## Done when
The tests `3d/point_lights` and `3d/point_light_marker` pass after the folder's build, and
`! grep -rn "point_light_strength\|flash_submit" 3d` exits 0.
