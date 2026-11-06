# 02 — A pass draws whole, or lays depth then shades
folder: render
after: 01
decisions: 0168, 0508

## Change
The public step the mesh draws of a camera pass are made in (0508 point 2), through card 01's
pipelines.

- `render/include/render/device.h`, beside `voe_render_frame_two_sided`:
  - `voe_render_depth_step` enum: `VOE_RENDER_DEPTH_STEP_WHOLE` (0, today's draw),
    `VOE_RENDER_DEPTH_STEP_LAY`, `VOE_RENDER_DEPTH_STEP_SHADE`.
  - `void voe_render_frame_depth_step(voe_render_device *device, voe_render_depth_step step)`:
    the solid and instanced mesh draws after it in this pass go through the solid, lay or shade
    pipeline. Points: every pass opens whole; lay writes depth where the cut-out and dither keep a
    pixel and no colour; shade tests EQUAL and writes no depth, so a group laid then shaded is the
    picture its whole draw makes, each kept pixel shaded once; a caller lays a run of draws, then
    shades the same run in the same order and two-sidedness, then sets whole. Asserts: no pass,
    no camera, a step not whole in a shadow, bounce shadow, point-shadow or capture pass.
  - `voe_render_frame_draw_blended`: asserts unless the step is whole.
  - `voe_render_frame_draw_instanced`, `_triangle_count`: a lay draw counts a draw, no triangles.
- `render/src/device_internal.h`: `depth_step` beside `cull_wanted`, meaningless with no pass.
- `render/src/pass.c`: a pass's open sets it whole, where it sets `cull_wanted`.
- `render/src/draw.c`: define the call. `draw_with` picks `pipeline`, `pipeline_lay` or
  `pipeline_shade` by the step for a solid draw in a camera pass, rebinding through `bound` as
  now; lay and shade take the dynamic cull as solid and blended do (the `cull_recorded` rule);
  shadow-kind passes untouched. Header: WHICH PIPELINE IS BOUND names the steps.
- `render/tests/depth_first.c`, new, headless; build its quads and records as
  `render/tests/cutout_far.c`, `dither.c` and `instances.c` do. Tests:
  - `laid_then_shaded_is_the_whole_picture`: two overlapping unlit cut-out quads of one-texel
    alpha stripes (cutoff 0.5), red far, green near, two-sided: whole, and lay both then shade both,
    read back the same bytes, both colours present.
  - `a_dissolving_pair_covers_each_pixel_once`: one quad at dither 0.5 and again at −0.5 in another
    colour, laid then shaded: every pixel exactly one of the two, the same bytes as whole.
  - `a_dither_below_the_floor_lays_nothing`: dither 0.01 laid then shaded: the clear colour.
  - `lay_alone_draws_no_colour`: a quad laid only: the clear colour; a whole quad behind it after
    is hidden where it was laid.
  - `instanced_copies_lay_then_shade`: three copies as in `instances.c`, the same bytes as whole;
    draw count two, triangles those of one draw.
  - `each_pass_opens_whole`: a pass ending in lay; the next pass's quad is drawn.
- Maps: `render/tests/tests.md` gains `depth_first.c`; `render/src/src.md` `draw.c` and `pass.c`
  entries name the step; `render/include/render/render.md` names the depth step.

## Meaning
- Holds: the LOD dissolve looks exactly as now: two groups with opposite dither in a band; the
  swap's incoming copy at a dither below the 1/32 floor covers no pixel. The second and third
  tests are this.

## Done when
`ctest --test-dir build/debug -R '^render/depth_first$'` exits 0 with the six tests above.
