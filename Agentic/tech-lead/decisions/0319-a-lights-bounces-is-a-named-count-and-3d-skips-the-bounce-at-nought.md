# 0319 — A light's bounces is a named count, and 3d skips the whole bounce at nought
date: 2026-10-02
by: planner

## Decision
For 047, carrying out 0316 and 0317 point 1 on 046's bounce:
1. **The field.** `voe_scene_light` gains `bounces`, a `uint32_t` described as UINT32, after the fill.
   `VOE_SCENE_LIGHT_BOUNCES_MAX` is 1 (051 raises it to 3). Its values are named "0" and "1" through
   `VOE_BASE_DESCRIBE_STRUCT_NAMED`, so the Inspector shows a dropdown of exactly the allowed counts with
   no editor change, and the scene text still holds a plain number. A count past the maximum is a bad
   light: `add` asserts, the drain refuses it, as for a negative intensity.
2. **Nought everywhere it is not chosen.** The default row, the untitled scene's light, dev's sun and a
   light read from a file without the key are all 0; the last is the reader's default-row rule, so no
   example scene is rewritten.
3. **The switch is 3d's.** `voe_3d_draw_system_shadows` opens the bounce pass, draws its casters and
   updates the target's grid only when the world's light (the row `voe_3d_draw_system_light` reads) has
   `bounces` of 1 or more. Render is unchanged: a camera pass reads a grid only when it was updated this
   frame, so at 0 lit surfaces take the fill floor alone, the look of before 046. The editor's views,
   its preview, the game and dev all go through that call, so they agree; a world with no light (the
   preview light) never bounces.
4. **Memory waits for 051.** At 0 there is no bounce pass, draw, update or read, but the bounce map and
   grids 046 builds with the device and each target stay allocated until 051 replaces them. 051's
   bounce allocates only when a light bounces.

## Reasoning
A named UINT32 is the existing dropdown path (0195, the shape's kind), so the Inspector, undo, saving and
the cook carry the field with no code of their own. The light row is the one thing every picture shares,
and the shadows call is the one place every caller opens the bounce, so gating there keeps editor and game
identical with one change.
- A `bounces` field on render's light record: render would gate a pass its caller already chooses to open.
- A free number box clamped by the drain: a drag past 1 refuses an intent every frame and prints each time.
- Building 046's images lazily now: placeholder grids in render's descriptor set and lazy target and map
  builds across five render files, all deleted by 051; images cost memory, not frame time.

## Replaces
Nothing. Carries out 0316 and 0317 point 1 for 046's bounce; point 4 defers 0316's "no image" to 051.
