# 05 — A removed caster marks where it stood
folder: 3d
after: 04
decisions: 0168, 0389, 0394

## Change
Carry out 0394 in 3d, so a deleted cube's tint leaves the floor (How to test step 5).

- New `3d/include/3d/bounce_casters.h`: `VOE_3D_BOUNCE_CASTERS` (512) and `voe_3d_bounce_casters`, a count
  and that many inline entries, each a `voe_ecs_entity` (`ecs/world.h`), a world centre as
  `voe_math_double3` about the world origin and a float radius. Zeroed is empty. Header: what it
  remembers and why (an entity's rows leave with it), one per target and the caller keeps it, only 3d
  writes it, NULL in the frame for none, past 512 a removal marks nothing, 20 KB.
- `3d/include/3d/draw_system.h`: `voe_3d_frame` gains `voe_3d_bounce_casters *casters`, after `target`,
  with a comment in the style of its neighbours (NULL none, _frame leaves it NULL, the caller sets it per
  target). The shadows call's doc (around "a removed caster marks nothing yet (0389)") says a removed
  caster marks where `frame->casters` remembers it, and nothing with NULL.
- `3d/src/draw_bounce.h`: declare `voe_3d_bounce_removed(world, device, frame, spheres, room)`, returning
  a count like `voe_3d_bounce_stale`: one sphere for each remembered entity that is no longer a caster
  this frame, at its remembered centre about the frame's eye, w its radius; then the memory holds this
  frame's casters at lag 0, centre and radius as `voe_3d_bounce_stale` computes them. Nothing with NULL.
  The header paragraph "A STALE SPHERE…" loses "a removed caster marks nothing".
- New `3d/src/bounce_casters.c` with its body. A caster is what `voe_3d_bounce_stale` walks; reuse its
  centre-and-radius code (read its body in `3d/src/draw_bounce.c`) by moving it into one helper both call,
  declared in `draw_bounce.h`, never copied.
- `3d/src/draw_bounce.c`, `voe_3d_draw_bounce`: the spheres from `voe_3d_bounce_removed` follow the
  stale ones in the same array, in the room left, before volume 0 begins. Header of the function in
  `draw_bounce.h` mentions them.
- Per-frame cost (0388): a CPU walk over the memory and the casters in the shadows call each frame a light
  bounces, no GPU pass. Say it in `bounce_casters.c`'s header and in the commit message.
- `3d/tests/bounce_world.inc`: the frame sets `casters` to a memory the world keeps.
- `3d/tests/bounce_tint.c`: case `a_removed_box_leaves_no_tint`: the purple cube settled, destroyed,
  settled; the 0.25 m ground within EVEN/255 of the reference. Then a new cube at the same place
  (the editor's undo makes a new entity), settled; tinted by at least TINT/255 again.
- Indexes, an entry each: `3d/include/3d/3d.md` (`bounce_casters.h`), `3d/src/src.md`
  (`bounce_casters.c`; `draw_bounce.h`'s line names the removed spheres), `3d/3d.md` if it lists the
  public headers, `3d/tests/tests.md` (`bounce_tint.c`'s cases).

## Done when
After `cmake --build --preset debug --target voe_3d voe_test_3d_bounce_tint voe_test_3d_bounce_scene`,
`ctest --test-dir build/debug -R '^3d/bounce_(tint|scene)$'` passes with `a_removed_box_leaves_no_tint`.
