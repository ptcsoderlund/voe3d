# 0336 — A model fades, and an enemy's wreck fades away with no collider
date: 2026-10-03
by: planner

## Decision
For 052 bug 01:
1. **A model row has a fade.** `voe_3d_model` gains `fade`, a `float` described as FLOAT32, last in
   its fields: 0 draws as today, 1 is gone, between is see-through by that much. The default row is
   0, so a literal, an old file and Add component are solid. Undo, save, prefabs and the cook carry it
   through the description; the Inspector shows it as every FLOAT32.
2. **The store gives every part a blended twin.** `voe_3d_model_part` gains `faded`, a shading
   record: for a `.glb` part whose material is not BLENDED, its material with the alpha mode BLENDED,
   uploaded beside its own at load and freed with it; for a part already blended, a picture's and the
   water's, its own `shading`. `VOE_3D_MODELS_SHADINGS` doubles to 1025.
3. **The draw.** A model row with `fade` at or above 1 draws nothing and casts nothing. Above 0 and
   below 1, each part is held in the world's blended group and drawn with its `faded` record and an
   object colour of (1, 1, 1, 1 − fade); it still casts as today. At or below 0, or not a number, it
   draws as today. Pick and outline are unchanged.
4. **The tank game's enemy wreck.** `enemy_wreck.prefab` has no collider, so tanks and shells pass
   through it. A game component `tank_fade_away` (`Tank / Fade away`): `wait`, default 2 s,
   `seconds`, default 1 s, read-only `age`. Each step while playing `age` grows by the step; every
   model under it and on it gets `fade` (age − wait) / seconds, clamped to 0..1, through the model's
   intent and only when it differs; at `age` ≥ wait + seconds the entity is removed with its tree.
   The enemy wreck's root carries it; the house wreck does not and stays solid.

## Reasoning
A field on the model is the path 0324 took for `cast_shadows`: one row per thing, saved and edited
with no code of its own, and zero is solid, so nothing written before it changes. `render` ignores
alpha in an opaque record (`device.h`), so a see-through part needs a BLENDED record; making it at
load keeps uploads between frames, as the store already promises, at one record per part.
- A separate engine fade component: two rows per thing for one number, refused as in 0324.
- Alpha honoured for every record in `render`'s shader: changes `render`'s contract for every caller.
- Twins made lazily on the first fade: the draw would upload mid-frame.
- Sinking or shrinking the wreck in game code alone: the report asks for see-through.
- Removing the collider in code at the swap: the prefab is the wreck's data, and the house's keeps its.

## Replaces
Nothing. Amends 0277 (the store's parts) and 0294 point 3 (the enemy's wreck is not solid and goes).
