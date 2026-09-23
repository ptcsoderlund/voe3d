# 03 — Authoring's tests read and write a lens
folder: authoring
decisions: 0168, 0222

## Change
Card 01 left the camera three fields (`fov_y`, `near_plane`, `far_plane`) and made it need a transform.
`authoring` reads and writes it through its description and names no field in `src/`, so only its tests follow.

- `authoring/tests/scene_read.c` — the world registers the transform before the camera. The scene texts'
  `[N.voe_scene_camera]` sections spell only lens fields, and the camera entity also carries a
  `[N.voe_scene_transform]` section. The checks that read `pitch`, `eye` or `yaw` read `fov_y` or `near_plane`
  instead; the refused-NaN case uses `fov_y = nan`. Where a test builds a camera with `voe_scene_camera_add`,
  the entity gets its transform first.
- Any other file under `authoring/tests/` that fails to build against `scene/include/scene/camera_component.h`
  follows the same way.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `authoring` exits 0.
