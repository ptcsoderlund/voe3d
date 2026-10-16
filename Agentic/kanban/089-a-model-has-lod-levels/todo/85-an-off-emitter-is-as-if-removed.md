# 85 — An off emitter is as if removed
folder: 3d
after: none
decisions: 0168, 0427, 0428, 0441

## Change
Bug 11, its particles half: an off emitter keeps spawning and moving its
particles unseen. The emitter system owns every particles row, and the
game, Play and the editor all step it through
`voe_3d_emitter_system_run`, so the fix is there, once (0441 point 1).

`3d/src/emitter_system.c` (304 lines; open its header comment and
`add_and_drop_rows`, about lines 146-163):
- `add_and_drop_rows` drops the row of an emitter whose entity
  `voe_scene_identity_off` (`scene/identity_component.h`) says is off, as
  it drops one whose emitter is gone, and adds no row to an off emitter.
  Since spawning and integrating walk the rows, an off emitter then spawns
  and moves nothing. A row `drain_controls` made for an off emitter's
  burst is dropped the same run, so the burst is lost (0441 point 1).
- Its comment says an off emitter is as a removed one (0441).
- The file's header gains "an off emitter has no row".

`3d/include/3d/emitter_system.h`: in A RUN, IN ORDER, the add-and-drop
point says an off emitter's row is dropped and none added while off, so
switched on it starts as a newly added emitter; the controls point says
play and stop still set `playing` on an off emitter. Signature unchanged.

`3d/src/draw_particles.c`: no change; its off test stays as the guard.

`3d/tests/emitter_system.c` (199 lines): `a_world` also registers the
identity table (`scene/identity_system.h`) and the parent table
(`scene/parent_system.h`), its limits raised to hold them; switch an entity
as `3d/tests/lights_switched_off.c`'s `authored_at` does (read its lines
37-100 only). New cases, in `main` and the file's header:
- `an_off_emitter_is_gone_and_starts_new`: a playing emitter with an
  identity runs a second: some live. Off and one run: no particles row.
  Thirty more runs: still none. A second entity given the same emitter
  just then; on and one run: both rows hold the same live count, none older
  than one step.
- `an_emitter_under_an_off_parent_is_gone`: the emitter on a child of an
  identity-carrying parent; the parent off and one run: no row; on: a row.
- `play_on_an_off_emitter_waits_for_on`: a stopped emitter with a burst of
  5, off; play submitted and a run: no row and the component says
  `playing`; on and a run: 5 live, as a newly added playing emitter.

`3d/tests/tests.md`: the `emitter_system.c` entry gains "an off emitter
gone and new again", one entry under 300 characters.

## Done when
The tests `3d/emitter_system` and `3d/draw_particles` pass after the
folder's build (the latter may skip without a graphics card).
