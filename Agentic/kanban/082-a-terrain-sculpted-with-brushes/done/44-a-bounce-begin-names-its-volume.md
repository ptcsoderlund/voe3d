# 44 — A bounce begin names its volume
folder: render
after: 43
decisions: 0168, 0387, 0389

## Change
The public half of 0389 point 1.

- `render/include/render/device.h`:
  - `struct voe_render_bounce_frame` gains `uint32_t volume`, below `VOE_RENDER_BOUNCE_VOLUMES`. Zero is the
    level grid, so every caller that names none is unchanged. The comment says what each index is for
    (0389).
  - `voe_render_bounce_begin`:
    - The assert "a target already begun this frame" becomes "this target's volume already begun this
      frame".
    - A volume at or past VOLUMES asserts.
    - Capture, shadow and relight calls act on the latest begin.
    - Volumes of one target are begun one after another, each with its own capture, shadow and relight
      calls.
  - New `[[nodiscard]] bool voe_render_bounce_placed(const voe_render_device *device, voe_render_target
    target, uint32_t volume, int32_t cell[3])`. It writes the lowest cell this volume was last placed at,
    and is false when the volume has never been placed, or was freed since.
  - The device-size paragraph near line 116 says a target holds up to four volumes, each about 43.6 MB,
    each one freed after 300 frames with no begin.
- `render/src/device_parts.h`: the begun record is per volume, so two volumes of one target begin in one
  frame.
- `render/src/bounce_volume.c`: the begin sets `device->bounce_volume`, builds and places that volume, and
  checks the per-volume begun. `voe_render_bounce_placed` lives here, reading the volume's probes.
- `render/tests/bounce_volume.c`, new cases:
  - `two_volumes_of_one_target_build_and_relight_apart`: begin volumes 0 and 3 of the window in one frame at
    spacings 2 and 1, then capture and relight each until settled. Both are built, and the placed cells
    read back.
  - `an_unplaced_volume_is_not_placed`
- `render/tests/tests.md`: the `bounce_volume.c` entry names them.

Per frame (0388): nothing more until 3d begins a second volume (card 54).

## Done when
`ctest --test-dir build/debug -R '^render/bounce_volume$'` passes with the two new cases.
