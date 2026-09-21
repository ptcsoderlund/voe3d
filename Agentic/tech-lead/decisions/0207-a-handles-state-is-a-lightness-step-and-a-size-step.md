# 0207 — A handle's state is a lightness step on the theme's colour and a size step

date: 2026-09-21
by: planner

## Decision
A gizmo handle is drawn in the lighter of the palette's `inverse` and `inverse_ink` — the one colour 0203
already establishes as visible on a scene view's near-black ground in every theme. At rest it is that colour
multiplied by a fixed fraction under one; hovered or held it is that colour undimmed and the handle is drawn a
fixed step wider. So the three states differ by lightness on the theme's own colour and by size, which is
0194's rule and 0196's inverted-when-held rule applied to something that is drawn on the engine's clear
colour rather than on a panel.

Hovered and held are drawn the same way. What tells them apart on screen is that a held handle is the only
one that is still marked once the pointer has travelled off it, and the thing it is attached to is moving.

## Reasoning
- `inverse` is what a held control is filled with and `inverse_ink` is the text on it, and which of the two is
  light depends on the theme's mode. A scene view is near-black whatever the theme, so the dark one of the
  pair cannot be the marked state: a handle nobody can see while dragging it is worse than no marking.
- Multiplying a linear colour by a fraction is a lightness step and keeps the theme's one hue, which is what
  0194 asks; it is not a derivation of a new role, which would be `ui`'s in OKLab (0170).
- A separate hovered and held colour would need a third role that no palette has, and inventing one in the
  editor is exactly what 0194 exists to stop.

## Replaces
Nothing. It applies 0194, 0196 and 0203 to the gizmo.
