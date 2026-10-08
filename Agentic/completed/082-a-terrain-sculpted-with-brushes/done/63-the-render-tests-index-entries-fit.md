# 63 — The render tests index entries fit
folder: render/tests
after: none
decisions: 0168

## Change
- `render/tests/tests.md`: these entries are each one sentence of 300 characters or fewer:
  `bounce_volume.c` (404), `bounce_settle.c` (342), `bounce_read.c` (434), `bounce_probes_scene.c` (313),
  `blocked_bounce.c` (495). A point an entry drops goes into that file's header only if the header does
  not already make it. Every header stays at 60 lines or fewer. They are 32 to 44 lines now. Change no code.

## Done when
`checks.sh --folder render/tests` reports no finding in `render/tests`.
