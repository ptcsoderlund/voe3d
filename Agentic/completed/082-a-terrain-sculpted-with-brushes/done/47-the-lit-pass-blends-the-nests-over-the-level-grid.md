# 47 — The lit pass blends the nests over the level grid
folder: render
after: 46
decisions: 0168, 0387, 0388, 0389

## Change
0389 point 6: every volume of the drawn target reaches the lit pass, and finer ones replace coarser ones
where they are ready, fading out over their edge.

- `render/src/device_parts.h`: the frame block's single `bounce` becomes
  `bounce[VOE_RENDER_BOUNCE_VOLUMES]`. Keep any size or offset assert true.
- `render/shaders/bindings.slangh`: the same array in the block (frame bounce struct near line 66, block
  near line 89), with a volume-count constant that matches C. A comment names the C side.
- `render/src/pass.c`: `name_volume` fills every entry for the pass's target. An entry's grid is the
  volume's descriptor index × 4 when that volume is built and begun this frame, and ~0u otherwise.
- `render/src/descriptors.c`: fix any assert on the block's size.
- `render/shaders/bounce_read.slangh`:
  - `voe_render_bounce_read` gains `float band` (cells) and `bool ready`, and returns `float4`.
  - rgb is the irradiance normalised over the weights that count, × readiness (validity G) when `ready`.
  - a is the edge fade × the sum of those weights. It is 0 when the sum is 0 or not finite.
  - The edge fade is the product over axes of saturate((min(u, size − u) − 0.5) / band).
  - The header says what a and band are.
- `render/shaders/lighting.slangh`, `voe_render_bounce_irradiance`:
  - E starts at 0. Volume 0, when named, gives rgb × a at band 1 with readiness.
  - Then each later named volume, in index order, makes E = lerp(E, rgb, a) at band 2 with readiness.
  - The header's bounce paragraph says so.
- `render/shaders/bounce_relight.slang`: its call reads band 1 without readiness and uses rgb × a, so a
  relight reads its own volume as before.
- `render/tests/bounce_read.c`:
  - Keep "nothing captured is no bounce".
  - New `an_empty_nest_reads_as_the_level_grid_alone`: the window's volume 0 settled; volume 3 begun at
    spacing 1 about the same place and built, nothing captured. The picture matches the one with volume 0
    alone, within 1 per channel.
  - New `a_settled_nest_reads_in_its_middle`: volume 3 settled too. The pixel under the nest's middle
    differs from the level-grid-only one.
- `render/tests/tests.md`: the entry names them.

Per frame (0388), the lit pass: three more trilinear reads per bounced pixel, only where a nest is named.

## Done when
`ctest --test-dir build/debug -R '^render/(bounce_|blocked_bounce|blocker_kinds_bounce)'` passes with the
two new cases.
