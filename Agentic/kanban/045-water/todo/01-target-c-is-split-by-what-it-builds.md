# 01 — target.c is split by what it builds
folder: render
after: none
decisions: 0168

## Change
`render/src/target.c` is 902 lines; card 02 adds to it. Split it by
function, moving code unchanged, each new file with its own header comment:

- `render/src/target.c` keeps the window's pair: the memory type, the image
  builds and teardowns, `voe_render_target_build` and `_teardown`, and the
  header's points about why the engine draws into images of its own.
- `render/src/target_own.c`, new: the targets of a caller's own (ADR-0148):
  `_startup`, `_shutdown`, `_at`, the settle, `voe_render_target_create`,
  `_resize`, `_apply_resizes`. Its header takes the caller-target points
  from target.c's.
- `render/src/target_read.c`, new: the read into RGBA8
  (`voe_render_target_read` and its helpers) and the points about it.
- A static helper both new files need becomes a declaration in
  `render/src/device_internal.h` beside the other target.c ones; the
  `// target.c.` prefixes there name the file each now lives in.
- `render/src/src.md`: the target.c entry narrowed, two new entries.

No behaviour changes; no public header changes.

## Done when
`wc -l render/src/target*.c` shows each under 450 lines, and the tests
`render/targets` and `render/offscreen` pass after the folder's build.
