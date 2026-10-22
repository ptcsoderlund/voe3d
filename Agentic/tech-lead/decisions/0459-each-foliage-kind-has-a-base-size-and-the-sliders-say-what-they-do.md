# 0459 — Each foliage kind has a base size, and the sliders say what they do
date: 2026-10-22
by: tech-lead

## Decision
For 090 bug 10.

1. **Each foliage kind gets its own base size**, a multiplier on the model's own size, 1× by
   default. It is set on that kind's row in the Foliage section, saved with the kind in the
   `.landscape`, and applies to every copy of that kind: changing it resizes the trees already
   painted as well as the ones painted later. A copy's size is base size × its own random
   factor. A landscape saved before this change opens with every kind at 1×.
2. **The random part stays a brush setting**, as before: each copy still draws its 1 ± vary
   factor when it is painted, and keeps it.
3. **The brush sliders are named for what they do:** Size becomes **Size ±%** (how far each
   copy's size may stray from the kind's base size), Turn becomes **Turn ±°** (how far each copy
   is spun at random), and Lean becomes **Tilt with slope %** (0 % stands straight up, 100 %
   stands square to the ground's slope). Their ranges and defaults do not change.

## Reasoning
The sponsor read "Size" as the trees' size and had no idea what "Lean" meant; the labels named
the stored numbers, not what the painter sees.
- A base size as a brush slider: it could paint big and small pines from one model, but a forest
  painted too small would have to be erased and repainted. Per kind, it is fixed in one place
  afterwards.
- Rescaling the model in Blender only: it works today, but it means a round trip out of the
  editor for what is a placement choice.
- Two kinds from one `.glb` at different base sizes are allowed if the sponsor wants big and
  small pines.

## Replaces
Amends 0442 points 1 and 3: `[Foliage]` carries a base size per kind, and the brush sliders are
renamed.
