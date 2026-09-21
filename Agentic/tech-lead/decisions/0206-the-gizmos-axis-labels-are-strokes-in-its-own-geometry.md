# 0206 — The gizmo's axis labels are strokes in its own geometry, not `text`

date: 2026-09-21
by: planner

## Decision
The X, Y and Z that tell the three arrows apart (0194) are drawn as part of the gizmo's own triangles: a few
camera-facing quads per letter — two crossed strokes for X, three for Y and three for Z — at the tip of each
arrow, sized as a fraction of the arrow like every other part of the gizmo. `text` is not called, no glyph
sheet is read and no material with a distance field is made for them.

## Reasoning
- A `text` block is a mesh and a texture wearing a distance-field material (`text/font.h`, ADR-0185), and one
  per axis per view per frame is three more transient ranges, three more materials and a second record kind
  for a pass that today draws one unlit white one. Three letters of three strokes are eight quads.
- The strokes scale, face the eye and stand in front of everything for free, because they are built by the
  same call, out of the same `metres`, as the arrows.
- The labels are a second cue and not reading matter: their whole job is that the arrow pointing away from the
  eye is still nameable. Legibility at four millimetres is `text`'s problem to have, not this one's.
- The day a gizmo needs a word rather than a letter, that is the day `3d` is given a text drawable, and this
  decision is the one it replaces.

## Replaces
Nothing. It reads 0194 for why a colour alone will not do.
