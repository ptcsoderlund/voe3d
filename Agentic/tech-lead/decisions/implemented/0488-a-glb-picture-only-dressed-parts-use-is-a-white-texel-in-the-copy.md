# 0488 — A `.glb` picture only dressed parts use is a white texel in the copy
date: 2026-10-28
by: planner

## Decision
0484 point 4 for models: a `.glb` picture that is used only by glTF materials the model's parts
dress with a `.material` (a non-empty `material<N>`, 0420) is not decoded, mipped or compressed.
The batch gives it a 1×1 white RGBA8 picture in its place, so the copy, the put and `3d` treat it
as any picture. The dressing is read from the `.parts` beside the `.glb`, already in the copy's
key, or in the game, which has none, from its parts table: `voe_game_models_update_cooked` takes
the `voe_game_parts` table `run.c` already holds.

## Reasoning
Import textures exports a `.glb`'s pictures as PNG maps for the `.material`s it writes and
dresses the parts with them; the `.glb`'s own pictures are then drawn by no part, yet every copy
decoded and compressed them a second time. Leaving them out of the copy is the only way the same
picture is not compressed twice without comparing pixels. A white texel keeps the copy's shape
and `3d` unchanged; a change of dressing changes the `.parts` stamp and makes the copy again.

## Replaces
Nothing.
