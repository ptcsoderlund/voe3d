# 0330 — The relight's sun map is drawn once per begun target, and each volume has its own relight record
date: 2026-10-03
by: planner

## Decision
For 051 bug 02, amending 0329 points 2 and 3:
1. **Once per begin, not per frame.** `voe_render_bounce_shadow_pass_begin` may open once after each
   `voe_render_bounce_begin`, so every view the editor shows draws the relight's sun map for its own
   volume. A second open after one begin still asserts. The slot keeps its one map: each view's
   pass, then its relight, are recorded in order, so the next view's pass waits on the last
   relight's compute read and overwrites the map. `drawn` is cleared by each begin, not by the
   frame's top.
2. **A relight record per volume.** The relight's uniform record, host-written, becomes one region
   per volume per frame slot, `(targets + 1)` of them at a stride rounded up to the card's uniform
   offset alignment, as the list buffer is banded; each volume's set names its own region. Two
   volumes relit in one frame no longer read the last one's placement, sun map and lamps.
3. **Cost.** One more pass and one more object per caster per view that relights a casting sun, as
   the editor's capacities already allow per view; no more memory for maps.

## Reasoning
The editor draws more than one view a frame, each with its own target and volume (0326 point 8). The
map was opened once per frame, so the second view's open asserted (bug 02). The record was one per
slot and written by the CPU at each relight, so it would have held the last view's values when the
GPU ran the first's dispatches.
- A map per volume: up to 4 MB × (targets + 1) a slot for no picture gain; the command order
  already serialises the views.
- Skipping the map for every view but the first: the others relight unshadowed.

## Replaces
Nothing. Amends 0329 points 2 and 3.
