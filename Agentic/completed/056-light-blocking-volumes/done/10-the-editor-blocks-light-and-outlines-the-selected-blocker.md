# 10 — The editor blocks light and outlines the selected blocker
folder: editor
after: 09
decisions: 0168, 0347
read: feature.md

## Change
Every editor view and the camera preview are lit through the blockers as the game is, and a view
draws the selected blocker's box (0347 point 5). Adding, editing, undo, parenting, prefabs and
saving need no editor code: the row is described, has a menu path and is drained in
`voe_game_world_step`. Read the headers of `3d/include/3d/draw_system.h`
(`voe_3d_draw_system_light_blockers`, `voe_3d_frame`'s `light_blocker`) and the files below.

- `editor/src/view_passes.c`:
  - The preview: before its point lights, `voe_3d_draw_system_light_blockers(world, &frame,
    arena)`; its pass camera passes `.blockers = frame.blockers`; no lines.
  - Each view: the same call before its point lights, `camera.blockers = frame.blockers` before the
    pass opens, and the frame handed to `voe_3d_draw_system_run` carries `.light_blocker` as
    `.collider` is: the selection, the outline's pixels and the view's size.
  - A false from the call leaves that pass unblocked and does not fail the frame.
  - The file's header names the blockers and the blocker's lines.
- `editor/src/view_passes.h`, `VOE_EDITOR_CAPACITIES`: the per-view transient vertices and indices
  gain another `VOE_3D_COLLIDER_MARKER_VERTICES` / `_INDICES`; the per-view objects term and the
  transient geometries per view each rise by 1; the reasoning comment says why.
- `editor/src/src.md`: the view_passes.c and view_passes.h entries name the blockers. Each at most
  300 characters.

## Done when
After the folder's build, `grep -c voe_3d_draw_system_light_blockers editor/src/view_passes.c`
prints 2 and `grep -q light_blocker editor/src/view_passes.c` exits 0.

The human's, in `examples/tank_game` in the editor (feature.md's How to test, all eight steps):
1. Walk into a house: bright from the fill.
2. Add entity, Add component "Rendering / Light blocker", size it to the house's inside: the inside
   goes dark (no fill, sun or bounce from outside); outside unchanged.
3. A point light outside, Cast shadows off, near a wall: lit inside before the blocker, not with
   it, at Bounces 0 and 3.
4. A point light inside: lights the room, its bounce fills it at Bounces 1+, nothing shows outside.
5. Make the house a prefab with the blocker as a child, spawn two, move one: each blocker follows.
6. Deselect: no box lines. Play: no box, the shell and tank pass through, the inside as in the
   editor.
7. Save, close, reopen: the blocker is there with its size.
8. A scene saved before this looks exactly as it did.
