# 0394 — A removed caster marks where it stood, from a memory the caller keeps per target
date: 2026-10-09
by: planner

## Decision
For 083, closing 0389 point 7's known gap ("a removed caster marks nothing"), which 083's How to test step 5
(delete the cube, the floor goes back to plain) needs.

1. **The memory.** 3d gains `voe_3d_bounce_casters` in a public header of its own
   (`3d/include/3d/bounce_casters.h`): up to `VOE_3D_BOUNCE_CASTERS` 512 entries, inline, no arena, each a
   caster's entity, its world centre in double about the world origin and its stale radius (0389 point 7),
   and a count. A caster is what `voe_3d_bounce_stale` walks.
2. **Where it goes.** `voe_3d_frame` gains a pointer to one, NULL for none. With none, a removed caster marks
   nothing, as before, so a caller that never sets it loses nothing.
3. **What 3d does with it.** In the bounce, each remembered entry whose entity is no longer a caster this
   frame (dead, without its mesh or model row, or no longer casting) marks one stale sphere at its
   remembered place and radius. Then the memory holds this frame's casters at lag 0. Past 512 casters the
   rest are not remembered, and their removal marks nothing.
4. **One per target.** A memory belongs to the target whose bounce it feeds, since each target has its own
   volumes: one per editor view and one for the preview, in the editor's views; one for the game's window,
   kept by the game's run loop and handed to `voe_game_frame`.
5. **Cost (0388).** A walk over the memory and the casters on the CPU, inside the shadows call, every frame a
   light bounces; no GPU pass. 20 KB a target.

## Reasoning
An entity's rows go with it when it is destroyed, the previous transform table's too, so nothing in the
world remembers where a removed caster stood. The bounce's caller already holds per-target state (the view,
the window) and passes the frame, so the memory rides there, mutated only by 3d.
- A destroy hook or a destroyed-entities list in ecs: wider than this feature, an event where data does.
- Recapture the whole nest when the caster count drops: thousands of probes for one pebble, against 0386.
- Keep the memory in render, fed every caster each frame: render would learn about entities it never names.

## Replaces
Nothing. Amends 0389 point 7: a removed caster now marks, where a memory is kept.
