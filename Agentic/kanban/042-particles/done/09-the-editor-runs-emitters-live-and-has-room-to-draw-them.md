# 09 — The editor runs emitters live and has room to draw them
folder: editor
after: 06, 07, 08
decisions: 0168, 0298

## Change
0298 points 4 and 8. Read `editor/src/world_step.h`, `editor/src/world_step.c`,
`editor/src/main.c` (split by card 08), `editor/src/view_passes.h` and
`3d/include/3d/emitter_system.h`. Pictures need nothing here: the editor's
model store loads them through `game/models.h` (card 07), and its failure
notice names them.

- `editor/src/world_step.h` and `.c`: `voe_editor_world_step` takes `float
  seconds` and runs `voe_3d_emitter_system_run(world, seconds)` after the
  game's world step and before the placed copies are expanded. The header
  says emitters run while editing so an effect is seen as it is tuned (0298),
  and that this is the one thing the editor steps by time; the "NO MOVE"
  paragraph stays true for bodies.
- `editor/src/main.c`: passes last frame's step, clamped by
  `MAX_FRAME_SECONDS`, kept across the loop (the world step comes before
  this frame's clock is read); 0 on the first frame and while a capture
  runs, so a capture's picture is the same each time. `MAX_FRAME_SECONDS`'s
  comment no longer says nothing integrates over time.
- `editor/src/view_passes.h`: `VOE_EDITOR_CAPACITIES.objects` adds
  `VOE_GAME_WORLD_EMITTERS * VOE_3D_EMITTER_PARTICLES * (VOE_EDITOR_VIEWS +
  1)`, a particle per view's pass and the preview's; the capacities
  paragraph in the header says so.
- `editor/src/src.md`: the `world_step` entry.

## Done when
`grep -q voe_3d_emitter_system_run editor/src/world_step.c` exits 0, and the
editor builds in the folder's checks.
