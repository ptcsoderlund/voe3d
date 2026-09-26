# 02 — Split the frame into its passes and its draws
folder: render
decisions: 0168

## Change
`render/src/frame.c` is 1146 lines and cards 03 and 04 add to it. Split it by function, no
behaviour change.

- `render/src/frame_internal.h` (new) — the calls the four files below share, with the
  `voe_render_` prefix their new linkage needs: `frame_at`, `image_at`, `bind_pools`,
  `open_rendering`, and whatever else a moved function reaches across. Header point: this is
  the seam between the frame, its passes and its draws; nothing outside these files includes
  it.
- `render/src/pass.c` (new) — `open_rendering`, `voe_render_pass_begin`, `_pass_end`,
  `_pass_is_open`, `voe_render_frame_set_viewport`. Header points: a pass is a rendering block
  onto a target with a camera block; the first pass onto a target clears, later ones load.
- `render/src/draw.c` (new) — `bind_pools`, `draw_with`, `voe_render_frame_draw`,
  `_draw_blended`, `_clear_depth`, `_draw_count`. Header points: one object record per draw;
  which pipeline is bound and why the pools are rebound only on change.
- `render/src/present.c` (new) — `ready_for_copy` and `blit_to_screen`. Header point: the one
  write into a swapchain image, last in a frame.
- `render/src/frame.c` — keeps begin, end, submit, rebuild, the GPU time, `frame_current`,
  `frame_open` and `_is_open`; its header narrowed to that and pointing at the three new files.
  The clear colour's comment stays with whichever file uses it.
- `render/src/src.md` — entries for the four new files; `frame.c`'s narrowed.

No public header changes.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder render` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^render/'` passes with no test file edited.
3. `wc -l render/src/frame.c render/src/pass.c render/src/draw.c render/src/present.c` shows
   each under 600.
