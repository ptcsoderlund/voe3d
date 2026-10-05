# 30 — A light inside a Fill box fills it
folder: render
after: none
decisions: 0168, 0361, 0350, 0357

## Change
Decision 0361 point 1 in the shaders. Read the header of `render/shaders/lighting.slangh` and its
`voe_render_fill_reaches`.

- `render/shaders/lighting.slangh`:
  - `voe_render_fill_reaches(mask, source)`: the Indoors test becomes "every Indoors bit of `mask`
    is also in `source`" in place of "no Indoors bit in `mask`". The Room test is unchanged. With
    `source` 0 it answers as today.
  - Its comment and the header's LIGHT BLOCKERS paragraph: the fill reaches in no Indoors box but
    those holding the light (0361).
- `render/shaders/shaders.md`: the `lighting.slangh` entry, if it states the fill gate.
- `render/tests/directional_lights.c`: a sixth case (`CASES` 6), in its header list. A Fill
  blocker (its bit in `indoors`) over the left half; the sun's mask 0, the moon's the Fill bit;
  both unshadowed. On the left the pixel in the box's shadow under the sun has blue from the moon's
  fill and no red from the sun's fill; on the right the sun's fill is there as in case 3.
- `render/tests/tests.md`: the `directional_lights.c` entry names the Fill case.

Expect `render/tests/blocked_light.c` to pass unchanged: its lights' masks are 0. If a case there
reads a light held by a Fill box, change that case's expectation to 0361's and say so in its header.

## Done when
`ctest --test-dir build/debug -R "^render/(directional_lights|blocked_light|water)$"` passes after
the render build.
