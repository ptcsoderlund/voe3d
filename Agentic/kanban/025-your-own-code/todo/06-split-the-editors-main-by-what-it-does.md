# 06 — Split the editor's main.c by what each part does
folder: editor
decisions: 0168

## Change
`editor/src/main.c` is 833 lines and cards 07, 09 and 10 add to its loop. Move code out
unchanged in behaviour; no new feature. Read main.c's header comment and the blocks named below.

- `editor/src/startup.h`, `editor/src/startup.c` (new) — which project opens: the folder on the
  command line or the remembered one or untitled, and the last-project write back (the blocks under
  "WHICH PROJECT OPENS" and "REMEMBERED FOR NEXT TIME"), and `say_whether_descriptions_are_in`.
  One call, e.g. `bool voe_editor_startup_project(const voe_editor_options *, voe_editor_session *)`
  (false: the argued folder would not open, already said on stderr), and the descriptions line.
  main.c's header paragraph "WHICH PROJECT IT OPENS ON" moves to startup.h's header; main.c keeps a
  one-line pointer to it.
- `editor/src/world_step.h`, `editor/src/world_step.c` (new) — the world's step once a frame:
  `void voe_editor_world_step(voe_ecs_world *, const voe_3d_shapes *)`, the structural queue
  applied, then the transform, identity and light systems, then the shape system, with main.c's
  comments on that order moved into its header (why before any reader, why all three always run).
  Card 07 adds the project's replaces here.
- `editor/src/main.c` — calls the two; its header lists them where it names its parts. If still
  over 700 lines, the capture (the frame count and the PNG write at the end) moves to
  `editor/src/capture.h`/`.c` the same way.
- `editor/src/src.md` — entries for the new files; main.c's entry if its sentence changes.

## Done when
1. `wc -l < editor/src/main.c` prints under 700.
2. Before the change, in `p=$(mktemp -d)` with `cp -r game/example/. $p`,
   `build/debug/editor/voe_editor --capture $p/before.png $p`; after it, the same into
   `$p/after.png`; `cmp $p/before.png $p/after.png` exits 0.
3. `build/debug/editor/voe_editor /nonexistent-folder` exits 1 with one `voe_editor:` line on
   stderr, before and after.
4. `checks.sh --folder editor` prints `FINDINGS: 0`.
