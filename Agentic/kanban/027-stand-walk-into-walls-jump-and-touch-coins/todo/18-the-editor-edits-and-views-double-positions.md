# 18 — The editor edits, moves and views double positions
folder: editor
decisions: 0168, 0250, 0254, 0177

## Change
`editor` mends what cards 01, 03, 08, 09 and 16 broke, and makes step 16 of `feature.md` hold in
the editor: 100 km out, a view up close does not shake and a typed position keeps its digits.

- `editor/src/inspector_value.h`, `inspector_value.c`, `inspector_edit.c`, `inspector.c` — the
  `DOUBLE3` kind shown as three number boxes like FLOAT3 (`lanes`, `is_vector`, the value read
  and its text), dragged and typed values written back as doubles; a typed 100000.001 reads back
  100000.001. Every switch on the kind names it.
- `editor/src/view.h`, `view.c` — the orbit's focus and `eye` are `voe_math_double3`; the pose
  handed to `voe_3d_view` carries the double eye; flying and orbiting add float motion to it.
- `editor/src/view_passes.c` — `voe_3d_draw_system_frame(world, size, 0.0f)` (0254: the editor
  draws what is); a view's own frame sets `eye` to its view's eye; the preview's is the world
  camera's.
- `editor/src/pick.c` — `voe_3d_pick_ray` with the view's eye.
- `editor/src/gizmo.h`, `gizmo.c` — `grab` and `start` are double3; the new position is the
  start plus the grab's difference, in double; `voe_3d_gizmo_at` with the view's eye.
- `editor/src/project.c` — the untitled scene's positions as doubles.
- `editor/src/game_tree.c`, `play.c`, `session.c`, `world_step.c` — only where a call above broke.
- `editor/src/src.md` and the touched headers — where one says a position or an eye is float.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints `FINDINGS: 0`.
2. In `p=$(mktemp -d)`: `examples/capsule/project.voe3d` and `main.scene` copied in (no `Code`,
   the lines naming the project's own components removed), every `position` X raised by 100000;
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0 and `$p/err` is
   empty; `$p/shot.png` shows the floor and the capsule (0177: the coder looks at it).
