# 58 — A probe inside geometry never darkens the read
folder: render
after: none
decisions: 0168, 0389, 0390

## Change
0390 amends 0389 point 6: a volume's weight a stops counting validity, so a box's own probes (inside it,
invalid) only renormalise the rgb and no longer darken the ground beside it. The read's signature and its
callers (`lighting.slangh`, `bounce_relight.slang`) do not change.

- `render/shaders/bounce_read.slangh`, `voe_render_bounce_read`:
  - rgb: unchanged. Its weights stay trilinear × validity (× readiness when `ready`) × blockers × facing
    × visibility, normalised; a weight sum of nought or not finite still reads all nought.
  - a: the edge fade × (Σ trilinear × validity × readiness) / (Σ trilinear × validity) when `ready`, and
    the edge fade alone when not. a is 0 when Σ trilinear × validity is 0 or not finite.
  - Header: the sentence on a (THE EIGHT PROBES) says what a now counts and that a probe inside or
    behind geometry only chooses what rgb is normalised over (0390); CONSTRAINTS no longer says a grid
    of mostly invalid probes reads dark, but that it reads its few valid probes at full weight.
- `render/shaders/shaders.md`: the `bounce_read.slangh` entry names a as edge fade × the valid probes'
  readiness (0390).
- `render/tests/bounce_read.c`:
  - New case `the_ground_beside_the_wall_is_redder_than_further_out`, in the THE NESTS scene with
    volume 0 alone settled (spacing 2; its probes are inside the wall or a metre off): on the ground
    at z 0, red less green 0.25 m out from the wall's sunlit +x face is greater than 3 m out.
  - Keep the existing cases; an empty nest still has a 0 (no valid probe).
  - Header: a paragraph for the new case, naming 0390.
- `render/tests/tests.md`: the `bounce_read.c` entry names the new case.

If a render bounce test other than these fails because it expected the darkened read, bring its
expectation to 0390 and say so in its header. If the new case fails with the read as above, block and
give both readings.

## Done when
`ctest --test-dir build/debug -R '^render/(bounce_|blocked_bounce|blocker_kinds_bounce)'` passes with
`the_ground_beside_the_wall_is_redder_than_further_out`.
