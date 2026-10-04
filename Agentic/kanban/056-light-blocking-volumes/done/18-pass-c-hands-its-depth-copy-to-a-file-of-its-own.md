# 18 — pass.c hands its depth copy to a file of its own
folder: render
after: none
decisions: 0168

## Change
`render/src/pass.c` is 812 lines and the next cards change it; split it by function first. No
behaviour changes.

- `render/src/depth_copy.c` (new): `depth_copy_barriers` (static) and
  `voe_render_frame_copy_depth`, moved whole from `render/src/pass.c` with the comments above
  them, and the includes they need. Header comment points: what the copy is (ADR-0305), that the
  block splits in two and the second loads, and that the slot is written into the pass's block —
  taken from what pass.c's header and the moved comments say.
- `render/src/pass.c`: the two functions gone; its header's paragraph on
  `voe_render_frame_copy_depth` becomes one line pointing at depth_copy.c; includes it no longer
  needs removed.
- `render/src/src.md`: a depth_copy.c entry; the pass.c entry no longer names the depth copy. Each
  at most 300 characters.

## Done when
`wc -l < render/src/pass.c` prints under 700, and the tests `render/passes` and
`render/depth_copy` pass after the folder's build.
