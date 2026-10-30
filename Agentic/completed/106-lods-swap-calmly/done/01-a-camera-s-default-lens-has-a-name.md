# 01 — A camera's default lens has a name
folder: scene
after: none
decisions: 0168, 0501

## Change
The 60° a new camera gets is a literal in its default row; the LODs panel (card 09) measures Swap
distance through it, so it gets a public name (0501 point 7).

- `scene/include/scene/camera_component.h`: add `VOE_SCENE_CAMERA_FOV_Y`, the default vertical field
  of view in radians (π/3, the value now in the default row), beside the fields' comment; the
  comment says it is what a new camera starts with and what a lens "by default" means elsewhere.
- `scene/src/camera_system.c`: the default row's `.fov_y` uses the constant.
- `scene/tests/camera.c`: a test `default_fov_is_the_named_lens` — a camera added to a fresh world
  reads `fov_y == VOE_SCENE_CAMERA_FOV_Y`; follow the file's existing test shape and registration.
- `scene/include/scene/scene.md`: the `camera_component.h` entry names the default lens constant.

## Done when
The test `scene/camera` passes and `grep -q VOE_SCENE_CAMERA_FOV_Y scene/src/camera_system.c` exits 0.
