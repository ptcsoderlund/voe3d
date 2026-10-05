# 0360 — Every directional light is marked in the views
date: 2026-10-05
by: planner

## Decision
For 058, bug 01. Replaces 0357 point 5's "the marker stays on the first light until 060".

1. A pass marks every directional light, not one. `voe_3d_sun_marked` takes the shape of
   `voe_3d_point_lights_marked`: `shown`, `selected`, `material`, `colour`, `selected_colour`,
   `pixels`, `size`. When `shown`, every light row with a transform is marked, in table order and
   past the fourth too (a light that does not shine is still placed and still picked, 0354): the
   selected one in `selected_colour`, the rest in `colour`, as two transient geometries, the rest
   together and the selected one alone. A pool too small draws none, as the point lights' rule.
2. The editor sizes its views' pools for `VOE_EDITOR_SUN_MARKERS` (16) lights. A scene with more
   shows no sun markers until it has fewer.
3. Picking is unchanged: `voe_3d_pick` already walks every light row's cube.

## Reasoning
- The point lights already solved "many markers, one selected" in one record; the same shape
  keeps one rule in `3d` and one pattern in the editor.
- 16 is four times what lights a frame, so a scene being built with spare lights is covered, for a
  few kilobytes of transient pool per view.

## Replaces
0357 point 5's last sentence. Extends 0274.
