# 03 — A shading record is written inside a frame
folder: render
after: 02
decisions: 0168, 0399

## Change
0399 point 4: a live material's record is rewritten without waiting for the card.

- `render/include/render/device.h` — under `shading`, new
  `void voe_render_shading_write(voe_render_device *device, voe_render_shading shading,
  voe_render_shading_values values)`: inside a frame and before its first pass; asserts outside a frame,
  after a pass began, or on a stale id. Its comment says the write is recorded into the frame's
  commands, ordered after earlier frames' reads by a barrier and before this frame's passes by another,
  so no frame in flight is waited for; the same order `voe_render_texture_write_heights` gives.
- `render/src/shading.c` — the record buffer gains transfer-destination use if it lacks it; the write
  records an update of the record's bytes (`vkCmdUpdateBuffer`, 80 bytes) between a barrier from
  fragment and vertex reads to the transfer and one back. Rewrite the header's "A RECORD IS WRITTEN
  ONCE" paragraph: created once between frames, rewritten in a frame.
- `render/src/texture_heights.c` — read its header for how its write asserts "inside a frame, before
  its first pass" and orders itself; the shading write uses the same check and order, through
  `render/src/device_calls.h` if the check is to be shared, never a copy of it.

Update `render/src/src.md`'s `shading.c` line.

New test `render/tests/shading_write.c` (follow `render/tests/heights.c` for a headless frame and
read-back): a quad drawn red, then in the next frame its record written blue before the pass, reads
blue; a write in each of three frames in a row reads the last.

## Done when
`ctest --test-dir build/debug -R '^render/shading_write$'` passes.
