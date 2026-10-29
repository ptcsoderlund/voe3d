# 11 — The editor's views show drifting fog and its boxes
folder: editor
after: 01, 07, 08, 09, 10
decisions: 0168, 0365, 0480, 0493

## Change
The editor steps the fog's drift as it steps the clouds', so patches drift while editing (0493
point 2); its views that build their own pass camera carry the fog from their own eye, as the
clouds; and every view lines the fog volumes' boxes as the blockers' (0493 point 7). Read the
headers of `editor/src/world_step.h`, `editor/src/world_step.c`, `editor/src/view_passes.h` and
`editor/src/view_passes.c` and, in the last, the lines naming `.clouds` and `.light_blockers`;
`voe_editor_view_faint_colour` in `editor/src/view.h`; `scene/include/scene/fog_volume_system.h`
(card 01); `voe_3d_fog_volumes` in `3d/include/3d/fog_volume.h` (card 07); the `fog` and
`fog_volumes` members of `voe_3d_frame` in `3d/include/3d/draw_system.h` (cards 08, 09).

- `editor/src/world_step.c`: `voe_scene_fog_volume_system_run(world, seconds)` right after the
  clouds'.
- `editor/src/world_step.h`: the paragraph on what runs while editing names the fog's drift
  beside the clouds'.
- `editor/src/view_passes.c`: wherever a pass camera's `.clouds` is set, `.fog` beside it: from
  `voe_3d_fog_volumes(world, view->eye, lag)` where the clouds come from the view's eye, at the
  lag that frame is given (0 when it sets none), from the frame's `fog` where they are the
  frame's; wherever `.light_blockers` is set, `.fog_volumes` beside it,
  shown, with the same selection, material, faint colour, selected
  colour, width and size.
- `editor/src/view_passes.h`: the header names the fog seen from the view's own eye beside its
  clouds, and the fog volumes' boxes beside the blockers'.
- `editor/src/src.md`: the world_step and view_passes entries follow, each at most 300 characters.

## Done when
After the folder's build, `grep -c voe_scene_fog_volume_system_run editor/src/world_step.c`
prints 1, `grep -c '\.fog = ' editor/src/view_passes.c` prints at least 2, and
`grep -c '\.fog_volumes' editor/src/view_passes.c` prints at least 1.

The human's, in the editor with the hill's project open, feature.md's How to test:
1. Add entity, Add component "Rendering / Fog volume", size it over part of the forest: fog lies
   among the trees in patches; none outside; no straight edge shows where it ends.
2. Move and resize it with the gizmos: the fog goes with it. Change Thickness and Patchiness.
3. Set Wind direction and speed: patches drift between the trunks and change shape as they go.
4. With a low sun, stand in the fog: beams fall between the trunks; turn away from the sun: they
   still show.
5. Add a large, thin volume, Patchiness 0, over the valley; with the Clouds drifting, look across
   it: beams through the cloud gaps, moving with the clouds, no fog banks.
6. Raise and lower Godrays strength on one volume: its beams strengthen and fade, the fog stays.
7. Switch both volumes off: only the haze is left.
8. Fly through the forest in fog: smooth; the Frame panel lists `View N: fog` with its ms.
9. Save, close and reopen: as left. Play, then Ship: the fog looks as in the editor's view.
