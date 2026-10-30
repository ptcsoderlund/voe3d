## Learned
- Play and a shipped build draw through one path, `voe_game_run` → `game/src/run.c` → `voe_game_frame`; dev sets no `frame.models`, so it draws no model LODs.
- The shader's dither needs no change for a timed dissolve: the object record's `dither` (and foliage `instance.dither`) already carries a fraction in (0,1] and its complement in [−1,0).
- A new camera's 60° lens was only a literal in `scene/src/camera_system.c`'s default row; nothing named it until 106.
- `3d/src/model_levels.c` measures the bounding radius (`radius_of`) the same way the store's `measure_sphere` does, so the panel and the store agree on the radius.
- Editor and game arenas are chains of blocks; a push never fails, so a multi-megabyte per-view memory on them is safe.

## Assumed
- The LOD memory holds 2^17 copies a frame, the foliage instance cap; past that a copy draws whole with no hysteresis.
- Shadows choose through the same memory as the camera pass; a second choose in one frame answers as the first, so the call order between shadows and draws does not matter.
- The Inspector's LOD row reads the view's memory and falls back to the stateless distance LOD before the first frame.
