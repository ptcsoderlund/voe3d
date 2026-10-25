# 13 — The Frame panel shows what textures take
folder: editor
after: 12
decisions: 0168, 0358, 0477

## Change
0477 point 7, and the last card: it makes `feature.md`'s How to test true.

- `editor/src/frame_breakdown.h`, `editor/src/frame_breakdown.c` — at the quarter-second take,
  `voe_render_texture_memory_smooth(gpu)` is kept beside the counts, from the device `_take` already
  has. A row under the totals reads `Textures` with the MB to one decimal, then `<n> of <m>
  compressed`. The header names the row.
- `editor/src/src.md` — the `frame_breakdown` entries.

## Done when
The editor builds, and `grep -n "voe_render_texture_memory_smooth"
editor/src/frame_breakdown.c` prints a line.

Human (How to test 1–6, in a release build, the hill project):
1. Open the hill. Note Textures in the Frame panel.
2. Copy the project, delete the copy's `Cache/` and open the copy. The splash shows "Preparing
   models" and "Compressing textures" counting. Afterwards Textures is a quarter or less of step 1,
   and every texture but the few emitter and picture ones counts as compressed.
3. Fly close to the ground, a log wall and a tree. They look as before, and the log bumps are smooth.
4. Close and reopen. No "Preparing" or "Compressing" shows.
5. Re-export a ground texture with a change. It shows within about a second. The compressed count
   drops by its maps and then climbs back within seconds, and the editor does not stall.
6. Ship and run the shipped game. Its textures are compressed: they look as in the editor, and it
   starts with no "Compressing textures".
