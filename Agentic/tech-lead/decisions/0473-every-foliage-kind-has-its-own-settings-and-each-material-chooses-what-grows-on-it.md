# 0473 — Every foliage kind has its own settings, and each material chooses what grows on it
date: 2026-10-23
by: tech-lead

## Decision
Foliage has no built-in classes: no "grass", "tree" or "rock" that behaves differently. The product's one
unit is the foliage kind, a row in a landscape's foliage list (the sponsor calls it a foliage layer). For
095:

1. **Grass is a kind.** A kind's look is a model or grass blades; blades are made as 0472 makes them.
   094's Inspector Grass section and `[Grass]`'s single "Grows on" layer give way to this.
2. **Every kind has the same settings**, chosen per kind: Cast shadows (0301; blades may now cast), base
   size (0469), the scatter sliders (0442), and Blend with ground (0467, 097), which a blades kind never
   has.
3. **Growing by itself belongs to the material.** Each material on a landscape's layers chooses which
   kinds grow on it, each with a density. Where it is painted, those kinds grow, thick where the paint is
   strong and thin where it fades; repainting changes them at once. A material choosing none grows none.
   Whether the choice is stored in the material's file or on the landscape's layer row is the planner's,
   as long as it is set where the material is.
4. **Erase leaves grown foliage alone.** Grown copies are the material's: they go only when the material
   is painted away or the kind is unticked on it. Hand Scatter and Erase (090) work as now on every kind,
   blades too, and touch only copies painted by hand.

## Reasoning
The sponsor decides what grows where, per material, and wants no foliage treated as special. One set of
settings for every kind is less to learn than a grass section beside a foliage list. Erase leaving grown
copies alone means repainting the ground never fights an erase the engine would have to remember.
Alternatives: grass alone choosing its materials (0472), which keeps grass special; the choice on each
kind instead of the material, which the sponsor turned down; Erase removing grown copies, which needs a
remembered mask that repainting overrules.

## Replaces
Nothing. Amends 0472 points 1, 5 and 8, and 0467's "never grass" now reads "never a blades kind".
