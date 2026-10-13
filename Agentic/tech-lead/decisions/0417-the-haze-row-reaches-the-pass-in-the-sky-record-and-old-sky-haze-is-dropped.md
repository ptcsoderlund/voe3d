# 0417 — The Haze row reaches the pass in the sky record, and old sky haze is dropped
date: 2026-10-13
by: planner

## Decision
For 087 bug 08, 0416 point 5.

1. **The record does not change.** `voe_render_sky` keeps `haze` and `haze_start` where they are, 96
   bytes. `3d`'s `voe_3d_draw_system_sky` fills them from the world's first Haze row (`haze`, `start`)
   and the rest from the first Sky row, each part zeroed when its row is missing. Neither row is the
   zeroed record, the old picture. The editor's views, which already take that record, need no code.
2. **`drawn` picks the hue.** In the shader, with `drawn` the haze colour is 0416 point 3's: `hue × B`,
   `hue` the sky's gradient along a level view (`view.y` 0) at strength 1 over its largest channel, white
   when that channel is not above 0. Without `drawn` it is 0415's `Σ c_i × I_i / (4π)`. `B` is 0416 point
   2's, over the same shown lights, with `VOE_RENDER_HAZE_GAIN`.
3. **The Haze row**: `voe_scene_haze`, fields `haze` and `start` (FLOAT32, each finite and at least 0),
   default row haze 1000 and start 100 (the sky's old defaults, so the unsaid row is the default), menu
   "Rendering / Haze", no needed type, one a game world.
4. **A sky row saved with `haze` or `haze_start` reads without them.** The scene reader ignores a key that
   is no field, with its warning, as for any other. No migration code: former names reach a field of the
   same type only, and the one saved scene with haze on its sky is the sponsor's hill
   (`lumberhill_project/main.scene`, `haze = 50`), which lives outside the repo with the sponsor's
   uncommitted work in it. The sponsor keeps its haze by moving that line under a new `voe_scene_haze`
   section of the same entity, or by adding Rendering / Haze at 50. The tank and coin games save no sky.

## Reasoning
- **One record, against a new `voe_render_haze`:** a new record is a new binding, offset asserts,
  pass-begin asserts and a new field on the pass camera and the editor's views, for two floats that
  already have a home the shader reads. The sky record already hazes whether or not `drawn` (0415).
- **The hue in the shader, against a hue computed in `3d`:** the record has no room for three more
  floats without growing; the shader already holds the gradient (0409).
- **Strength 1 in the hue:** at strength 0 the gradient is black, which would make the hue white; 0416
  says strength does not reach the haze.
- **No migration:** code read once, for one file, kept forever; the sponsor's step is one line.

## Replaces
Carries out 0416 point 5. 0414 point 3's record and asserts stand; `3d` copies `start` into `haze_start`.
