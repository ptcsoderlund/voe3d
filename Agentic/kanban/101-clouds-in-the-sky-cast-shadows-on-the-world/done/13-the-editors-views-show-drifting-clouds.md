# 13 — The editor's views show drifting clouds
folder: editor
after: 01, 11, 12
decisions: 0168, 0418, 0480, 0492

## Change
The editor steps the clouds' drift as it steps water, so they drift while editing (0492 point
2), and its views that build their own pass camera carry the clouds from their own eye, as the
sky. Read the headers of `editor/src/world_step.h`, `editor/src/world_step.c`,
`editor/src/view_passes.h` and `editor/src/view_passes.c`, `scene/include/scene/clouds_system.h`
(card 01) and `voe_3d_draw_system_clouds` in `3d/include/3d/draw_system.h` (card 11).

- `editor/src/world_step.c`: `voe_scene_clouds_system_run(world, seconds)` right after water's.
- `editor/src/world_step.h`: the paragraph on what runs while editing names the clouds' drift
  beside emitters and water, and "the only things the editor steps by time" follows.
- `editor/src/view_passes.c`: wherever a pass camera's `.sky` is set, `.clouds` beside it: from
  `voe_3d_draw_system_clouds(world, view->eye)` where the sky is taken from the view's eye, from
  the frame's `clouds` where the sky is the frame's.
- `editor/src/view_passes.h`: the header names the world's clouds seen from the view's own eye
  beside its sky.
- `editor/src/src.md`: the world_step and view_passes entries follow, each at most 300 characters.

## Done when
After the folder's build, `grep -c voe_scene_clouds_system_run editor/src/world_step.c` prints 1
and `grep -c '\.clouds' editor/src/view_passes.c` prints at least 2.

The human's, in the editor with the hill's project open, feature.md's How to test:
1. Add component "Rendering / Clouds" to an entity; set Kind to Volumetric: clouds over the valley.
2. Slide coverage 0 to 1 and back; change height and thickness: they clear, fill, move and grow.
3. Set wind direction and speed: they drift that way; their shadows sweep over the hill, the
   forest and the house, matching the clouds above.
4. Change Shadow strength: cloud shadows lighten and darken, the house's and trees' own shadows
   stay. Change the light's fill: object shadows change, cloud shadows do not.
5. Lower the sun and make it orange: the clouds turn orange where lit, darker on the far side.
6. Fly up from the tower to the clouds, round them and into them: they have depth.
7. Look at the water and at a shiny surface: the clouds show in them.
8. Set Kind to Flat: a flat drifting layer, still casting shadows. Set it back.
9. Fly through the forest under them: smooth; the Frame panel lists `View N: clouds` with its ms.
10. Save, close and reopen: as left. Play, then Ship: they look as in the editor's view.
