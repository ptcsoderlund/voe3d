# 33 — Each probe volume has its own relight record in a frame slot
folder: render
after: none
decisions: 0168, 0326, 0330

## Change
0330 point 2, for bug 02 of 051: the relight's uniform record is one per frame slot and written by
the CPU at each relight, so two volumes relit in one frame (the editor's views) would both read the
last one's. Read `bounce_relight.c`'s header first.
- `render/src/device_parts.h`: `struct relight_record` moves here from `bounce_relight.c` as
  `struct voe_render_relight_record`, with its comment and its offset and size static asserts, so a
  test can read a region.
- `render/src/device_internal.h`: the device gains `relight_record_stride`, the record's size rounded
  up to the card's uniform offset alignment as `pass_stride` is; the comment on `relight_records`:
  `(targets + 1)` regions a slot, volume n's at n × the stride.
- `render/src/bounce_relight.c`: the record buffers are built that size; the stride set at startup;
  the relight writes its volume's region (the volume's index as the list buffer's band uses it), and
  binding 11 of that volume's set names that region alone (offset and the record's size). Header:
  the PER SLOT paragraph says one record per volume, why (two volumes relit in one frame, 0330).
- `render/tests/bounce_volume.c`: a case after the window's and a target's volumes are built: one
  frame begins both at different cells, with a sun of bounces 1, and relights each after its begin;
  read through `device_internal.h`, that slot's region 0 holds the window's cell and the target's
  region its own. Header paragraph for the case.
- `render/src/src.md` (`bounce_relight.c`), `render/tests/tests.md` (`bounce_volume.c`): entries.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_"` passes.
