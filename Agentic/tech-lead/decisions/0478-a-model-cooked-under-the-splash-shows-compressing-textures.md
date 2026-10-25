# 0478 — A model cooked under the splash shows "Compressing textures"
date: 2026-10-25
by: tech-lead

## Decision
While a splash cooks a `.glb` again, and so compresses its pictures, the splash reads "Compressing
textures" with the models' done/total count. Before that it reads "Preparing models", and once the
model is put it moves on to the next file's phase. The count moves once per model; it does not count
pictures or blocks inside one model. The cook before Play and Ship (`voe_game_models_cook`) still shows
"Preparing models". The materials' maps show "Compressing textures" as 0477 point 3 says. Fix 002 did
this, in `game/`.

## Reasoning
The human testing 096 saw "Loading models 2/3" stand still for minutes while one large model's
textures were compressed in a debug build, and read it as stuck. Naming the work tells them the
editor is busy, not hung.
- A count of pictures or blocks inside a model: rejected for now, since the picture loop is in `3d/`,
  which has no progress type, so it would touch two folders and needs a work order.
- Leaving "Preparing models": rejected, since that is the phase the human took for a hang.

## Replaces
Amends 0477 point 3 ("Preparing models" for a model).
