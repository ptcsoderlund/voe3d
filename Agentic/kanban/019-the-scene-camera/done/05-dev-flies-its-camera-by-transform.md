# 05 — The dev program flies its camera by transform intents
folder: dev
decisions: 0168, 0222, 0223

## Change
Card 01 took the camera's pose and its place and motion intents away; the dev program keeps its own flight and
submits the eye's transform.

- `dev/src/motion.h` / `motion.c` — new `voe_dev_flight { voe_math_float3 eye; float yaw; float pitch; }`.
  `voe_dev_camera_motion` becomes `voe_dev_flight voe_dev_fly(voe_platform_window *, voe_dev_flight, float
  seconds)`: the same keys and mouse as today, applied as the camera system applied a motion before card 01
  (read it in `git show 1e65424:scene/src/camera_system.c`), its constants moved here unchanged
  (3 m/s, ×4 fast, 0.004 rad per unit, pitch limit 1.5533431). `voe_dev_orbit(float seconds)` returns a
  `voe_dev_flight`. New `voe_scene_transform voe_dev_flight_pose(voe_dev_flight)`: position the eye, rotation
  yaw about +Y then pitch about the turned X (0223), scale one. Header follows: what main.c submits now.
- `dev/src/startup.h` / `startup.c` — the eye entity gets a transform (from the orbit at 0 s) before its camera;
  the camera value is the lens only; the transform is registered before the camera.
- `dev/src/main.c` — keeps the flight in the program's loop state; each frame it is flown or orbited (the
  handover starts the flight from where the orbit had got to, as today), and its pose is submitted with
  `voe_scene_transform_submit` for the eye. `voe_scene_camera_system_run` stays in the loop (it drains lens
  intents). Header paragraphs on the camera system moving the eye follow. A frame with `blind` set is still
  opened and closed; `voe_3d_draw_system_run` already draws nothing then.
- `dev/src/facing.h` / `facing.c`, `dev/src/sprites.h` / `sprites.c` — the three facing calls take
  `voe_scene_transform eye` (this frame's pose, handed by main.c) instead of an eye entity read from the camera
  table, forward, right and up from its matrix's columns; `voe_dev_sprites_face` takes the `voe_dev_flight` and
  turns by its yaw. Their headers' "one frame behind" reasoning becomes: handed this frame's pose, so
  nothing lags (0223).
- `dev/src/monitor.h` / `monitor.c` — the monitor's camera becomes `voe_scene_transform pose` and
  `voe_scene_camera lens`, the pose built with `voe_dev_flight_pose` from its eye and two angles; its frame's view
  comes from `voe_3d_view` (`3d/include/3d/projection.h`). The header's "not an entity" paragraph stays.
- `dev/src/src.md` — the motion, facing, sprites and monitor entries follow.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `dev` exits 0, and
`grep -rnE "camera_place|camera_move|camera_forward|camera_motion|camera_placement" dev` prints nothing.
