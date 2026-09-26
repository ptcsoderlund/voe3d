# 27 — The editor's views open on the scene's camera, proven
folder: editor
decisions: 0168, 0255, 0250, 0177
read: feature.md

## Change
Card 22's change is committed (7194550): `voe_editor_views_focus_camera` in
`editor/src/view.h`/`view.c`, its two calls in `editor/src/main.c`, and the `editor/src/src.md`
entries. It blocked only on two findings outside `editor`, which cards 25 and 26 fix. Edit
nothing unless a proof below fails; if one does, the fix is in those same four files:
the views' focus set to the world camera's transform position (the origin when none), the eye
placed again by `orbit_place`, yaw, pitch and distance untouched; called once after
`voe_editor_views_create` and when `session.replaced` is acted on, not on Refresh or Play.

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
