# 05 — A pass draws its fog after the clouds
folder: render
after: 04
decisions: 0168, 0305, 0358, 0480, 0492, 0493

## Change
0493 point 5: the call that marches and composites a pass's fog, sharing the clouds' sequence and
image. Read `render/src/clouds_draw.c` (whole: its sequence moves), the headers of
`render/src/frame_internal.h` and `render/src/target.c`; in `render/include/render/device.h` the
comments of `voe_render_frame_draw_clouds` and `voe_render_fog`; the target record's `clouds`
comment in `render/src/device_parts.h`; and `render/tests/clouds.c` as the test pattern.

- `render/src/half_march.c` (new): `[[nodiscard]] bool voe_render_frame_half_march(...)`, taking
  the march and composite pipelines and the span's name: clouds_draw.c's sequence moved here
  whole, inside that span (the depth copy if unset, the block ended, the frame slot's clouds image
  to attachment, the half-size march, back to shader read, the block reopened, `clouds_image`
  written, the composite, `bound` left, not counted). Declared in `frame_internal.h`, whose list of
  files names it. Header: what, the order, why the block splits, that the clouds and then the fog
  march into the one image, so its second use waits on the first's composite reads by the barrier
  into attachment.
- `render/src/clouds_draw.c`: the asserts and the kind-none skip, then the helper with the clouds'
  pipelines and `clouds`; its header says the sequence is the helper's. No picture changes.
- `render/include/render/device.h`: `[[nodiscard]] bool voe_render_frame_draw_fog(
  voe_render_device *device)` with its comment: draws the open camera pass's fog volumes over what
  it holds; called right after `voe_render_frame_draw_clouds` and before the overlay's depth
  clear, once a pass; copies depth first when the pass has not; a pass whose fog count is 0
  records nothing and is true; timed as the span `<pass>: fog`; the same asserts as the clouds'
  call; false only when the depth copy is. `voe_render_fog`'s comment points here.
- `render/src/fog_draw.c` (new): the call, the count read back from the pass's block, then the
  helper with the fog's pipelines and `fog`. Its header.
- `render/src/device_parts.h`: the target's `clouds` comment says the fog marches into it after
  the clouds, resting in shader read outside either march.
- `render/tests/fog.c` (new), headless, a target looking along −Z under a drawn sky and one light,
  a volume of size 20³ centred 30 m ahead, density 0.05, patchiness 0, godrays 0:
  count 0 draws the same bytes as no call and lists no `fog` span; the volume changes the centre
  pixel by more than 8/255 and lists `<pass>: fog`; density 0 is within 1/255 of none; a pixel
  whose view misses the box is within 1/255 of none; an opaque quad 5 m ahead covering the centre
  keeps its pixels within 1/255 of none; the eye inside the volume changes the centre too; clouds
  and fog in one pass both draw, the frame's spans listing both.
- `render/src/src.md`, `render/tests/tests.md`, `render/include/render/render.md`: an entry or a
  phrase each, at most 300 characters.

A validation or Best Practices message from the image's second use fails the checks: fix the
barrier, never allowlist it.

## Done when
The tests `render/fog`, `render/clouds`, `render/depth_copy` and `render/pass_times` pass after
the folder's build.
