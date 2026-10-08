# 43 — A target has room for four probe volumes
folder: render
after: 42
decisions: 0168, 0387, 0389

## Change
Make room for 0389 point 1's four volumes per target. Only volume 0 is begun yet, so behaviour does not
change. The next card lets a begin name a volume.

- `render/include/render/device.h`: `VOE_RENDER_BOUNCE_VOLUMES` 4, beside the other `VOE_RENDER_BOUNCE_*`.
  Its comment says it is the level grid and three nests (0389).
- `render/src/device_parts.h`: `voe_render_target_slot.volume` becomes an array of
  `VOE_RENDER_BOUNCE_VOLUMES`.
- `render/src/device_internal.h`:
  - `window_volume` becomes an array of `VOE_RENDER_BOUNCE_VOLUMES`.
  - New `uint32_t bounce_volume`, beside `bounce_target`: the volume of the current begin.
- `render/src/device_calls.h`:
  - `voe_render_bounce_volume_of(device, target)` gains `uint32_t volume`.
  - The descriptor index of a volume becomes slot × `VOE_RENDER_BOUNCE_VOLUMES` + volume, where the window
    is slot 0 and target n is slot n. One inline helper says so, and every user calls it.
- `render/src/descriptors.c`:
  - Binding 6 holds (targets + 1) × VOLUMES × 4 entries; binding 10 holds (targets + 1) × VOLUMES.
  - Fix the assert near the volume write.
- `render/src/bounce_relight.h`: `volume_count` counts every volume. The relight's sets, lists and record
  regions are indexed by the volume's descriptor index (`bounce_relight_build.c`, `bounce_relight.c`).
- `render/src/bounce_volume.c`, `bounce_capture.c`, `bounce_shadow.c`, `bounce_relight.c`:
  - Every `voe_render_bounce_volume_of` call names `device->bounce_volume`.
  - Build, idle-free and rebuild walk every volume of a slot.
- `render/src/device.c`: teardown frees every volume of the window and of each target.
- `render/src/frame.c`: anything that walks a slot's volume walks all of them.
- `render/src/pass.c`: `name_volume` names volume 0's descriptor index through the helper.
- `render/src/src.md`: index entries change only where a file's one line names "the volume".

Each header comment that says "a target's volume" says the target's volumes, with a phrase on the index.

## Done when
`grep -c "VOE_RENDER_BOUNCE_VOLUMES" render/src/descriptors.c` prints at least 1. Run
`ctest --test-dir build/debug -R '^render/(bounce_|blocked_bounce|blocker_kinds_bounce)'` after a build: it
passes unchanged.
