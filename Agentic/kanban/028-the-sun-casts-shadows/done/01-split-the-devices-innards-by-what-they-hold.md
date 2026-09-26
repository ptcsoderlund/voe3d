# 01 — Split the device's innards by what they hold
folder: render
decisions: 0168

## Change
`render/src/device_internal.h` is 884 lines and cards 03 and 04 add to it. Split it first, no
behaviour change.

- `render/src/device_parts.h` (new) — the records the device is built of, moved as they are:
  `struct voe_render_frame_block`, `_buffer`, `_pool`, `_transient_pool`, `_geometry_slot`,
  `_shading_slot`, `_texture_slot`, `_image`, `_allocated_image`, `_target`, `_target_slot`
  and `struct voe_render_frame`, with the constants only they read. Header points: what each
  record is a part of, and that the device struct and the calls between files stay in
  `device_internal.h`.
- `render/src/device_internal.h` — keeps its header, the constants the whole folder reads,
  `struct voe_render_device` and the cross-file calls; includes `device_parts.h`. Header: the
  file list names `device_parts.h`.
- `render/src/src.md` — an entry for `device_parts.h`; `device_internal.h`'s narrowed.

No public header changes; no other file edited.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder render` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^render/'` passes with no test file edited.
3. `wc -l render/src/device_internal.h render/src/device_parts.h` shows each under 700.
