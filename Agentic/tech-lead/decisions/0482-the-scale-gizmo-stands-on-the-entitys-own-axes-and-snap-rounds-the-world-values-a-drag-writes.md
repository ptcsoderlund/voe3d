# 0482 — The scale gizmo stands on the entity's own axes, and snap rounds the world values a drag writes
date: 2026-10-27
by: planner

## Decision
Feature 099 (0438) is built so:

1. **The scale gizmo is `3d`'s arithmetic** (`3d/gizmo_scales.h`), as the arrows and rings are
   (0205, 0274), but along **the entity's own axes**, its world rotation handed in: three shafts
   one shaft long, each ending in a camera-facing square (half side 0.08 shaft), and a
   camera-facing square at the centre (half side 0.12 shaft); no labels. Handles X, Y, Z and a
   new `VOE_3D_GIZMO_CENTRE`. Hit nearest along the ray, an axis within `VOE_3D_GIZMO_GRIP` of
   its segment beyond the centre square.
2. **A scale drag is measured from the press.** An axis grabs the point of its line nearest the
   ray; the factor is the signed distance along the axis now over at the press. The centre grabs
   where the ray meets the plane through the origin facing the eye; the factor is 1 plus the
   travel along that plane's direction nearest the world's up, in shafts (looking straight along
   Y, the direction nearest world −Z). Either factor is at least 0.01. The world scale at the
   press times the factor, on that axis or on all three, replaces the world scale, submitted as
   a whole transform as moves and turns are.
3. **The gizmo's mode is one enum**, `voe_3d_gizmo_kind` (ARROWS nought, RINGS, SCALES) in
   `3d/gizmo.h`, replacing `rings` in `voe_3d_gizmoed` and in the editor's scene. R cycles it.
4. **Snap arithmetic is `3d`'s** (`3d/gizmo_snap.h`), pure and tested: a position's world axes
   the handle drags (X: x; XY: x and y; …) to the nearest multiple of the step, the others
   untouched; a swept angle wrapped to (−π, π] then to the nearest multiple; a scale's
   components the handle scales (CENTRE: all three) to the nearest multiple, never below one
   step. Snap rounds the world values the drag writes, so a parented child's row may read off
   the step. Turns are whole steps from the press. Typed values go nowhere near it.
5. **The snap state is the person's** (`voe_editor_snap`: on, move m, rotate degrees, scale),
   held on the editor's scene beside the gizmo's kind, kept in `editor_settings` as `snap` (`0`
   or `1`), `snap_move`, `snap_rotate`, `snap_scale` (`%.3f`); in range move 0.001–1000, rotate
   0.1–360, scale 0.001–100; defaults off, 0.5, 15, 0.1. Written when the toggle flips, a step
   is typed, or a step box's drag ends; never per frame.
6. **Ctrl is read as a level every frame of a drag**: snapping is on XOR Ctrl held.
7. **The top bar** after the mode label: a Snap choice drawn selected while on, then the three
   steps as number boxes showing their figure and unit.

## Reasoning
A scale is per the entity's own axis; a world-axis handle on a turned box would stretch it along
some other line than the one dragged. A factor measured from the press cannot creep (0205's
reason). Snapping the world values keeps 0438's world grid for the move and treats the other two
the same way, one rule. Arithmetic in `3d` is testable; the editor has no tests.
- Local values snapped: a child's world place off the grid, against 0438.
- Centre factor from distance to the centre: a press on the centre has none to shrink from.
- Ctrl latched at the press: a hand that reaches for Ctrl a moment late gets nothing.

## Replaces
nothing. Extends 0205, 0274, 0438.
