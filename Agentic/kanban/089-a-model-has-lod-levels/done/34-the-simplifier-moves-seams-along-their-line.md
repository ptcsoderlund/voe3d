# 34 — The simplifier moves seams along their line and softens edges
folder: 3d
after: none
decisions: 0168, 0425, 0430

## Change
0430 points 1 and 2: the in-house simplifier (card 03) never moved a seam, so a flat-shaded or
uv-split model barely shrinks. Seams now move along their line, and normals within an angle share a
side. Internal to `3d`; the one caller is `3d/src/model_levels.c`.

- `3d/src/simplify.h`: `voe_3d_simplify` gains `float edge_degrees` after `max_error`. Header, the
  TOPOLOGY paragraph rewritten for 0430 point 1: a position's vertices fall into sides (equal uv and
  colour complement, normals within `edge_degrees` of the side's first vertex; 0 means equal); one
  side moves as before (free, or along a border); two sides with exactly two seam edges and no border
  edge move only along a seam edge, each side's corners onto the vertex the same side's triangle on
  that edge names at the kept position; anything else never moves. A seam edge: an edge of two
  triangles naming different sides at either end. A soft position's corners take the kept vertex.
  The usage example passes the angle.
- `3d/src/simplify.c`: the per-position kind gains the seam line; sides found after the weld; a seam
  edge counted per position as the border count is; a seam-line collapse proposed only along its seam
  edges and its corners re-pointed by side, the collapse refused when a moved corner's side is named
  by neither edge triangle. The kept-vertex rule for one side is as now. Header: the CONSTRAINTS and
  the corner paragraph name the sides. 562 lines now; if it would pass ~800, move the weld and the
  sides into `3d/src/simplify_sides.c` with its header, declared in a small internal header.
- `3d/src/model_levels.c`: its call passes 0. Nothing else changes here (card 35's).
- `3d/tests/simplify.c`: existing calls pass 0. Case 4 (the grid's uv seam column) now asserts: the
  column's two end positions still used, and no written triangle names vertices of both copies of the
  column. New cases:
  5. a flat-shaded open cylinder of 16 sides and 8 rings along its length, each side its own strip
     of 2 × 9 vertices wearing the side's normal, no caps; to 96 indices at angle 0, `max_error`
     1e-3: at most 96, error under 1e-5 (the rings fall along straight seam lines);
  6. that cylinder to 48 indices, `max_error` its radius: at angle 0 it writes 96 or more; at angle
     180 fewer than at angle 0, error above nought and under the radius.
  The header lists the six.
- `3d/tests/tests.md`: the `simplify.c` entry names seam lines and the angle. `3d/src/src.md`: the
  `simplify.h` and `simplify.c` entries name seam lines and soft edges, one sentence each.

## Done when
The test `3d/simplify` passes after the folder's build, with cases 5 and 6 in it.
