# 06 — The capture pass: sixteen probes' pictures in one layered pass
folder: render
after: 05
decisions: 0168, 0325, 0326

## Change
0326 point 3, 0325's layered drawing with the probes as its lights. Read `point_shadow.c`'s and
`draw.c`'s headers first: this pass is that one with colour.
- `render/include/render/device.h`: `[[nodiscard]] bool voe_render_bounce_capture_pass_begin(
  voe_render_device *device, bool *opened)`, beside the point-shadow pass. Comment: takes up to
  `VOE_RENDER_BOUNCE_CAPTURE` queued probes of the begun target, nearest the eye first, and opens
  one pass drawing their six faces each; `opened` false when none is queued, the volume is not
  built, or `VOE_RENDER_BOUNCE_CAPTURE_PASSES` ran this frame; a pass, counted against `passes`
  and its draws against `objects`, false with a line when spent; asserts outside a frame, with a
  pass open or with no begin this frame. `voe_render_frame_draw`'s comment: in a capture pass,
  as in a point-shadow pass, one instanced draw over the faces reached within the reach.
- `render/shaders/draw.slang`: `capture_vertex`, the point-shadow vertex stage with the light's
  range as `VOE_RENDER_BOUNCE_REACH`, also passing the world normal, the position about the
  probe and the uv; `capture_fragment` writing albedo (base factor × texture × object colour, as
  the bounce map's fragment does) and (world normal, distance from the probe). Both through
  `point_shadow.slangh`'s face projection.
- `render/src/pipeline.c`: a capture pipeline: those entries, nothing culled, depth `GREATER`,
  two colour attachments (RGBA8 sRGB, RGBA16F), no blend; only with `output_layer`.
- `render/src/bounce_capture.c`, new: per frame slot a scratch of 96 layers of
  `VOE_RENDER_BOUNCE_FACE` square, the two colours and a D32 depth, built at startup with the
  device when `output_layer`; the pass begin (card 04's take, the probes written as the pass's
  point lights by slot, centre about the eye, range the reach; colours cleared to nought with
  distance the reach, depth cleared), and its end: barriers, then for each probe its six layers
  copied into its 48 × 8 strip of both atlases (probe (i, j, k) at x 48i, y 8(12k + j), face f at
  + 8f), then back to GENERAL. Header: why a scratch and a copy (an atlas of 6912 probes cannot
  be layers).
- `render/src/device_parts.h`, `device_internal.h`: the pass kind, the scratch, the calls.
- `render/src/pass.c`: `voe_render_pass_end` calls the capture end for this kind.
- `render/src/draw.c`: the capture pass takes the point-shadow path, mask over the pass's probes
  at the reach, through the capture pipeline; `_draw_blended` and `_clear_depth` assert in it.
- `render/src/element.c`, `pass.c`'s depth copy: refused in it as in a point-shadow pass.
- `render/src/device.c`: the scratch built and torn down.
- `render/tests/bounce_capture.c`, new, headless: a volume built, the frame's begin with a cube
  2 m from the eye: the first capture pass opens and the cube adds one draw, a cube 100 m off
  none; the fifth in one frame does not open; with `passes` spent it is false; a cube's red
  reaches the albedo atlas strip of the nearest probe (read the atlas through the internal
  header and a copy to a host buffer, or the scratch before the copy).
- `render/src/src.md`, `render/shaders/shaders.md`, `render/tests/tests.md`: entries.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_capture$"` passes.
