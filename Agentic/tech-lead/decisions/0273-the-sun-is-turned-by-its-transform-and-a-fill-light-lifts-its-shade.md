# 0273 — The sun is turned by its transform, and a fill light lifts its shade
date: 2026-09-27
by: planner

## Decision
For 035 (0268 milestone 2):

1. **The sun's direction is its transform.** The light component loses `direction`. The light
   travels along its entity's transform rotation's −Z, the camera's convention (0033) and
   glTF's for a directional light. The light needs a transform (`ecs` needs, as the camera's);
   a light row with no transform shines along −Z. `scene` gives the two conversions: a
   rotation into the unit direction it shines, and a direction into the shortest-arc rotation
   that shines it, for code that holds a direction.
2. **The light gains a fill:** `fill_colour` (COLOUR) and `fill_intensity` (FLOAT32), default
   white and 0. `colour` becomes a COLOUR field; the scene text spells it as before.
3. **The fill is a flat ambient term.** `render`'s light record gains `fill`, linear, the fill
   colour times its intensity, set by `3d`. The shader adds `fill × base colour` to every lit
   surface, unshadowed and from no direction. Zero is no fill, so a zeroed record draws as
   before. An `unshaded` pass reads none. This amends 0258 point 5's "no ambient".
4. **The light's intent is its replace.** The Inspector edits it live; the drain keeps the last
   valid row, with one stderr line, when a number is not finite, a colour channel is outside
   [0, 1] or an intensity is negative.
5. **There is still one sun.** The editor's Duplicate refuses an entity with a light, as it
   refuses the camera; a second light still asserts in `3d` (0238 leaves more for later).

## Reasoning
0222 and 0271 already say a transform is the one thing that places or turns anything in 3D
space, and that the sun can be parented; a direction field would be a second rotation the
rotate gizmo, undo and parenting all have to special-case. Keeping `direction` and deriving the
marker's turn from it was rejected for that reason. A hemisphere or image-based fill was
rejected as more than the feature asks: one flat term keeps shadows (only the sun is
shadowed) and is the smallest step light bounce (milestone 13) replaces. Default fill 0 keeps
every existing picture and test as it is.

## Replaces
Amends 0258 point 5 (a fill term now exists) and scene/light_component.h's direction field.
