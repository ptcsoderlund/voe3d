# 0404 — A model's file holds its parts' materials, and the material gains cut-out, double-sided and emission
date: 2026-10-11
by: tech-lead

## Decision
Settles 0403's open questions, built in work order 088. Which `.material` each part of an imported model
wears is kept in a small file of the model's, written beside the `.glb` by the import and shared by every
placed copy of it; changing a part's material changes every copy. The per-entity Material 1–8 rows of the
model component go, and nothing replaces them per entity until a game needs two copies to differ. The
standard material gains three things: cut-out (a threshold below which a pixel is not drawn, solid when
far away), double-sided (seen from both sides), and emission (a colour and strength, and an emission map
slot). The import fills them from the glTF material, and Import textures puts the emission picture into
the emission slot as it does colour, normal and ORM; maps with no slot are still written as PNGs and
attached to nothing. Animations remain their own decision.

## Reasoning
- The tech-lead's recommendation, taken by the human to try and revisit once seen.
- Part materials per entity, as now: lets two copies of a house differ, but nothing asks for it, and the
  rows confuse on every model and on landscapes, where they do nothing.
- Emission left unattached as 0403 said: less work, but downloaded models' glowing parts would not glow,
  and 088's How to test already promises they do.
- See-through blending is not added here; 088 asks for cut-out, not glass.

## Replaces
Amends 0403: emission is no longer "attached to nothing"; it fills the new emission slot. Amends 0399: the
model component's `materials` paths move into the model's own file.
