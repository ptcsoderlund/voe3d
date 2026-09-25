# 22 — The editor's views open on the scene's camera
folder: editor
decisions: 0168, 0255, 0250, 0177
read: feature.md

## Change
Feature step 16 in the editor: with the scene 100 km out, the views open where it is (0255).
Card 18's double positions, eye and gizmo are in the tree and stay as they are.

- `editor/src/view.h`, `view.c` — a new
  `void voe_editor_views_focus_camera(voe_editor_views *views, const voe_ecs_world *world)`:
  every view in use gets its focus set to the world camera entity's transform position
  (`voe_scene_camera_entities`/`_count` in `scene/camera_component.h`,
  `voe_scene_transform_get` in `scene/transform_component.h`), the origin when there is none,
  and its eye placed again by the orbit (`orbit_place`); yaw, pitch and distance untouched.
  Header points: when it is called (startup, a different project) and not (Refresh, Play), and
  why (0255); `voe_editor_views_create`'s comment no longer says the views look at the origin
  for good, only until this is called.
- `editor/src/main.c` — the call once after `voe_editor_views_create` succeeds, and in the branch
  that acts on `session.replaced` (beside `voe_editor_undo_forget`), with `scene.world`.
- `editor/src/src.md` — `view.h`/`view.c` and `main.c` entries: the views open on the camera.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints `FINDINGS: 0`.
2. In `p=$(mktemp -d)`: `cp -r examples/capsule/. $p`, `rm -rf $p/Build`, then in
   `$p/main.scene` every `position = [x, y, z]` line's x raised by 100000 (awk):
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is empty,
   and `$p/shot.png` shows the level, capsule and camera marker as the same capture of the
   unmoved `examples/capsule/` does, from the same angle, with no smeared or broken geometry
   (0177: the coder captures both and looks at them).
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's: `feature.md`'s `## How to test` step 16 in the editor on `examples/capsule/`
   (every X raised by 100000 in the Inspector, the views reopened by Open on the saved project,
   nothing shakes up close in the editor or in Play).

## Blocked
The editor change is in and Done when 1 and 2 pass (both captures identical, no stderr), but `checks.sh --all` prints `FINDINGS: 2`, both from before this card and outside `editor`: `3d/include/3d/3d.md` does not list `collider_marker.h`, and `scene/tests/tests.md`'s `transform.c` entry is 322 characters (cap 300). A card for `3d` and one for `scene` fixing those two entries would unblock it; nothing in `editor` is left to do.
